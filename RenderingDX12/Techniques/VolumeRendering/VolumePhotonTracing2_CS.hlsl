#include "../CommonGI/Parameters.h"
#include "../Randoms/RandomHLSL.h"

cbuffer Volume : register(b0) {
	int3 Dimensions;
	float Density;
	float Absortion;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
	float3 LightDirection;
}

cbuffer Transforms : register(b2) {
	row_major matrix AccProjMatrix;
	row_major matrix AccViewMatrix;
};

cbuffer PathtracingInfo : register(b3) {
	int PassCount;
};

Texture3D<float> Data	: register (t0);
Texture3D<float> Accum	: register (t1);

sampler VolumeSampler	: register(s0);
sampler AccSampler		: register(s1);

struct Photon
{
	float3 Position;
	float3 ViewContribution;
};

RWStructuredBuffer<Photon> Photons : register(u0);

#include "PhaseAniso.h"

void GetVolumeBox(out float3 minim, out float3 maxim) {
	float maxC = max(Dimensions.x, max(Dimensions.y, Dimensions.z));
	minim = -Dimensions * 0.5 / maxC;
	maxim = -minim;
}

float3 TransformFromWorldToAccum(float3 wpos) {
	float4 p = mul(mul(float4(wpos, 1), AccViewMatrix), AccProjMatrix);
	return float3(0.5 * p.xy / max(0.01, p.w) + 0.5, p.z);
}

// Assumes P is inside bMin-bMax, return the t parameter to hit a box side
float BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = abs(D2) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	return max(0, min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12)));
}

float3 GenerateStartPosition(float3 bMin, float3 bMax, float3 L, out float pdf) {
	float3 bDim = bMax - bMin;
	float3 areas = bDim.zxy * bDim.yzx * abs(L);
	float totalArea = dot(areas, 1);
	pdf = 1 / totalArea;
	float selectedSide = random() * totalArea;
	float3 c1 = L < 0 ? bMin : bMax;
	float3 c2 = c1;
	if (selectedSide < areas.x) // choose yz plane
	{
		c1.yz = bMin.yz;
		c2.yz = bMax.yz;
		return lerp(c1, c2, float3(0, random(), random()));
	}
	if (selectedSide < areas.x + areas.y) // choose xz plane
	{
		c1.xz = bMin.xz;
		c2.xz = bMax.xz;
		return lerp(c1, c2, float3(random(), 0, random()));
	}
	c1.xy = bMin.xy;
	c2.xy = bMax.xy;
	return lerp(c1, c2, float3(random(), random(), 0));
}

void PhotonTrace(int photonIndex, float3 L, float3 Intensity) {

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float3 V = normalize(AccViewMatrix._m02_m12_m22);

	float pdf;
	float3 P = GenerateStartPosition(bMin, bMax, L, pdf);
	float3 D = -normalize(L);

	float3 sizes = bMax - bMin;

	float3 LightIncomming = Intensity / pdf; // (sizes.x * sizes.y + sizes.y * sizes.z + sizes.x * sizes.z);

	float d = BoxIntersect(bMin, bMax, P, D); // start with distance traversing the volume box

	while (true) {

		float majorant = Density;
		float t = -log(1 - random()) / majorant; // Using Density as majorant
		float tpdf = exp(-t * majorant) * majorant;

		if (t >= d)
			return; // Photon went outside the volume

		P += t * D; // Update photon position

		float3 tSamplePosition = (P - bMin) / (bMax - bMin);
		float densityt = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

		float nullProb = Density - densityt;

		float prob = 1 - nullProb / Density;

		if (random() < prob) // scattering
		{
			float3 accSamplePos = TransformFromWorldToAccum(P);
			float transmitanceToViewer = Accum.SampleGrad(AccSampler, accSamplePos, 0, 0);

			float3 photonContribution = LightIncomming * EvalPhase(D, -V) * exp(-transmitanceToViewer * Density);

			float probToSave = photonContribution / (Photons[photonIndex].ViewContribution + photonContribution);

			if (random() < probToSave) {
				Photons[photonIndex].Position = P;
				Photons[photonIndex].ViewContribution = photonContribution / probToSave;
			}
			else
				Photons[photonIndex].ViewContribution *= 1 / (1 - probToSave);

			float scatterPdf;
			float3 scatterD;
			GeneratePhase(D, scatterD, scatterPdf);

			D = scatterD;

			d = BoxIntersect(bMin, bMax, P, D);
		}
		else
			d -= t;
	}
}

float3 SampleSkybox(float3 L) {
	float azim = L.y;
	//return lerp(float3(0.9, 0.95, 1), float3(0.1, 0.2, 0.5), -sign(azim)*0.5 + 0.5);
	return lerp(float3(0.7, 0.8, 1), float3(0.5, 0.3, 0.2), 0.5 - azim * 0.5);
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint dim, stride;
	Photons.GetDimensions(dim, stride);
	
	Photons[DTid.x] = (Photon)0; // clear photon

	StartRandomSeedForThread(dim, 1, DTid.x, 0, PassCount);

	//float3 globalLightD = randomDirection();
	//PhotonTrace(DTid.x, globalLightD, SampleSkybox(globalLightD) * 4 * pi);
	PhotonTrace(DTid.x, LightDirection, LightIntensity);
}
