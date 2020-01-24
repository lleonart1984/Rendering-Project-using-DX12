#include "../CommonGI/Parameters.h"
#include "../Randoms/RandomHLSL.h"

cbuffer Camera : register(b0) {
	row_major matrix FromProjectionToWorld;
};

cbuffer Volume : register(b1) {
	int3 Dimensions;
	float Density;
	float Absortion;
}

cbuffer Transforms : register(b2) {
	row_major matrix FromVolToAcc;
}

cbuffer Lighting : register(b3) {
	float3 LightPosition;
	float3 LightIntensity;
	float3 LightDirection;
}

cbuffer PathtracingInfo : register(b4) {
	int PassCount;
};

Texture3D<float> Data : register(t0);
Texture3D<float> Light : register(t1);

sampler VolumeSampler : register(s0);

RWTexture2D<float3> Accumulation	: register(u0);
RWTexture2D<float3> Output			: register(u1);

//#include "PhaseIso.h"
#include "PhaseAniso.h"

float3 SampleSkybox(float3 L) {
	float azim = L.y;
	return lerp(float3(0.7, 0.7, 1), float3(0.2, 0.1, 0), 0.5 - azim * 0.5);
}

void GetVolumeBox(out float3 minim, out float3 maxim) {
	float maxC = max(Dimensions.x, max(Dimensions.y, Dimensions.z));
	minim = -Dimensions * 0.5 / maxC;
	maxim = -minim;
}

bool BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMin, inout float tMax)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = D2 == 0 ? float2x3(float3(-100000, -100000, -100000), float3(100000, 100000, 100000)) : C / D2;
	tMin = max(max(min(T._m00, T._m10), min(T._m01, T._m11)), min(T._m02, T._m12));
	tMin = max(0, tMin);
	tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
	if (tMax <= tMin+ 0.0001 || tMax <= 0.0001 || tMin > 1000) {
		return false;
	}
	return true;
}

void DensityIntegral(float stepscale, float3 beg, float3 end, float sampleT, float3 bMin, float3 bMax, out float T1, out float T2, out float3 total)
{
	float3 D = end - beg;
	float totalDistance = length(D);
	total = 0;
	T1 = 0;
	T2 = 0;

	if (totalDistance < 0.00001)
		return;

	D /= totalDistance;

	float3 V = -D;
	float3 L = normalize(LightDirection);

	float step = stepscale / max(Dimensions.x, max(Dimensions.y, Dimensions.z)); // half of a voxel size

	float t = 0;
	while (t + step < totalDistance)
	{
		float3 samplePosition = (beg + D * t - bMin) / (bMax - bMin);

		// Do something with the Voxel
		float density = Data.SampleGrad(VolumeSampler, samplePosition, 0, 0) * Density;

		float3 lightPosition = mul(float4(samplePosition, 1), FromVolToAcc).xyz;
		float3 toLightDensity = Light.SampleGrad(VolumeSampler, lightPosition, 0, 0) * Density;

		float sigmaA = density * Absortion;
		float sigmaS = density * (1 - Absortion); // scattering
		float sigmaT = sigmaA + sigmaS;

		total += LightIntensity * step * exp(-T2 - step * sigmaT * 0.5) * sigmaS * exp(-toLightDensity) * EvalPhase(V, L);

		T1 += step * sigmaT * (t <= sampleT);
		T2 += step * sigmaT;

		// Move to next step
		t += step;
	}
}

float3 Lin1(float3 P, float3 D)
{
	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float tMin = 0;
	float tMax = 1000;
	float3 V = -normalize(D);
	float3 L = normalize(LightDirection);

	if (BoxIntersect(bMin, bMax, P, D, tMin, tMax)) {
		float T1, T2;
		float3 total;
	
		DensityIntegral(1, P + D * tMin, P + D * tMax, 0, bMin, bMax, T1, T2, total);
		
		//return exp(-T1) * densityt * (1 - Absortion) * exp(-toLightDensity) * LightIntensity / (4 * 3.14159) / tpdf +
		return total + exp(-T2) * SampleSkybox(D);
	}

	return SampleSkybox(D); // Background
}

float3 Lin(float3 P, float3 D)
{
	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);
	float tMin = 0;
	float tMax = 1000;
	float3 V = -normalize(D);
	float3 L = normalize(LightDirection);

	if (BoxIntersect(bMin, bMax, P, D, tMin, tMax)) {
		//float t = random() * (tMax - tMin); // sampling between 0..d
		//float tpdf = 1 / (tMax - tMin);

		float t = -log(1 - random());
		float tpdf = exp(-t);

		float T1, T2;
		float3 total;
		DensityIntegral(0.5, P + D * tMin, P + D * tMax, t, bMin, bMax, T1, T2, total);

		float3 tSamplePosition = (P + D * (tMin + t) - bMin) / (bMax - bMin);

		float densityt = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

		float3 lightPosition = mul(float4(tSamplePosition, 1), FromVolToAcc).xyz;
		float3 toLightDensity = Light.SampleGrad(VolumeSampler, lightPosition, 0, 0) * Density;

		//float3 scatterD = randomDirection();
		//float scatterPdf = 1 / (4 * 3.14159);

		float scatterPdf;
		float3 scatterD;
		GeneratePhase (V, scatterD, scatterPdf);

		//return exp(-T1) * densityt * (1 - Absortion) * exp(-toLightDensity) * LightIntensity / (4 * 3.14159) / tpdf +
		return
			exp(-T1) * densityt * (1 - Absortion) * Lin1(tSamplePosition, scatterD) / tpdf //* EvalPhase(V, scatterD) / scatterPdf
			+ total + exp(-T2) * SampleSkybox(D);
	}

	return SampleSkybox(D); // Background
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	Output.GetDimensions(dim.x, dim.y);

	StartRandomSeedForRay(dim, PATH_TRACING_MAX_BOUNCES + 1, DTid.xy, 0, PassCount);

	float4 ndcP = float4(2 * (DTid.xy + 0.5) / dim - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	Accumulation[DTid.xy] = (Accumulation[DTid.xy] * PassCount + Lin(O, D)) / (PassCount + 1);
	Output[DTid.xy] = Accumulation[DTid.xy];
}

