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

sampler VolumeSampler	: register(s0);
sampler LightSampler	: register(s1);

RWTexture2D<float3> Accumulation	: register(u0);
RWTexture2D<float3> Output			: register(u1);

//#include "PhaseIso.h"
#include "PhaseAniso.h"



float3 SampleSkybox(float3 L) {
	//return float3(0, 0, 1);

	float3 BG_COLORS[5] =
	{
		float3(0.00f, 0.0f, 0.02f), // GROUND DARKER BLUE
		float3(0.01f, 0.05f, 0.2f), // HORIZON GROUND DARK BLUE
		float3(0.7f, 0.9f, 1.0f), // HORIZON SKY WHITE
		float3(0.1f, 0.3f, 1.0f),  // SKY LIGHT BLUE
		float3(0.01f, 0.1f, 0.7f)  // SKY BLUE
	};

	float BG_DISTS[5] =
	{
		-1.0f,
		-0.04f,
		0.0f,
		0.5f,
		1.0f
	};
	float3 col = BG_COLORS[0];
	for (int i = 1; i < 5; i++)
		col = lerp(col, BG_COLORS[i], smoothstep(BG_DISTS[i - 1], BG_DISTS[i], L.y));
	return col;
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
	float2x3 T = abs(D2) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	tMin = max(max(min(T._m00, T._m10), min(T._m01, T._m11)), min(T._m02, T._m12));
	tMin = max(0.0, tMin);
	tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
	if (tMax <= tMin || tMax <= 0) {
		return false;
	}
	return true;
}

void BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMax)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = abs(D2) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	tMax = max (0, min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12)));
}

//bool BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMax)
//{
//	float3 C = bMax - P;
//	float2x3 T = abs(D) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D;
//	tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
//	if (tMax <= 0) {
//		return false;
//	}
//	return true;
//}

void ComputeIntegrals(float stepscale, float3 beg, float3 end, float sampleT, float3 bMin, float3 bMax, out float T1, out float T2, out float3 total)
{
	float3 D = end - beg;
	float totalDistance = length(D);
	total = 0;
	T1 = 0;
	T2 = 0;

	if (totalDistance < 0.001)
		return;

	D /= totalDistance;

	float3 L = normalize(LightDirection);

	float step = stepscale / max(Dimensions.x, max(Dimensions.y, Dimensions.z)); // half of a voxel size

	float t = 0.5 * step;
	while (t < totalDistance)
	{
		float3 samplePosition = (beg + D * t - bMin) / (bMax - bMin);

		// Do something with the Voxel
		float density = Data.SampleGrad(VolumeSampler, samplePosition, 0, 0) * Density;

		float3 lightPosition = mul(float4(samplePosition, 1), FromVolToAcc).xyz;
		float3 toLightDensity = Light.SampleGrad(VolumeSampler, lightPosition, 0, 0) * Density;

		float sigmaA = density * Absortion;
		float sigmaS = density * (1 - Absortion); // scattering
		float sigmaT = sigmaA + sigmaS;
		
		T1 += step * sigmaT * (t <= sampleT);
		T2 += step * sigmaT;
		total += LightIntensity * step * exp(-T2) * sigmaS * exp(-toLightDensity) * EvalPhase(D, L);
		
		// Move to next step
		t += step;
	}
}

float3 Pathtrace2(float3 P, float3 D) {

	float3 accum = 0;
	float importance = 1;
	int bounce = 0;

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float tMin = 0;
	float tMax = 1000;
	bool intersectVolume = BoxIntersect(bMin, bMax, P, D, tMin, tMax);

	if (!intersectVolume) // traspased volume => Ld no scattering
		return accum + importance * SampleSkybox(D);

	while (bounce < 2) {
		float t = -log(1 - random()) / Density;
		float tpdf = exp(-t*Density)*Density;

		float T1, T2;
		float3 totalLight;
		ComputeIntegrals(0.5 * (1 << (2 * bounce)), P + D * tMin, P + D * tMax, t, bMin, bMax, T1, T2, totalLight);

		accum += importance*(totalLight + exp(-T2) * SampleSkybox(D));

		if (t >= tMax - tMin)
			return accum;

		float3 scatterP = P + D * (tMin + t);

		float3 tSamplePosition = (P + D * (tMin + t) - bMin) / (bMax - bMin);
		float densityt = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

		importance *= exp(-T1) / tpdf;

		float scatterPdf;
		float3 scatterD;
		GeneratePhase(D, scatterD, scatterPdf);

		P = scatterP;
		D = normalize(scatterD);

		tMin = 0;
		tMax = 1000;
		BoxIntersect(bMin, bMax, P, D, tMin, tMax);

		bounce++;
	}

	return accum;
}

float3 Pathtrace(float3 P, float3 D) {

	float3 accum = 0;
	float3 importance = 1;

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float tMin = 0;
	float tMax = 1000;
	BoxIntersect(bMin, bMax, P, D, tMin, tMax);

	P += D * tMin; // Put P inside the volume

	float d = tMax - tMin;

	int bounces = 0;

	float pdf = 1;

	while(true)
	//while (bounces < Absortion*200) 
	{
		float t = -log(max(0.000000001, 1.0 - random())) / Density; // Using Density as majorant

		if (t >= d)
			//return pdf;
			return SampleSkybox(D) + accum;

		P += t * D;

		float3 tSamplePosition = (P - bMin) / (bMax - bMin);
		float prob = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0);

		if (random() < prob) // scattering
		{
			float3 lightPosition = mul(float4(tSamplePosition, 1), FromVolToAcc).xyz;
			float toLightDensity = Light.SampleGrad(LightSampler, lightPosition, 0, 0) * Density;

			accum += exp(-toLightDensity) * LightIntensity * EvalPhase(D, LightDirection);

			float scatterPdf;
			float3 scatterD;
			GeneratePhase(D, scatterD, scatterPdf);

			D = scatterD;

			tMax = 1000;
			BoxIntersect(bMin, bMax, P, D, tMax);

			d = tMax;

			bounces++;
			pdf = 1;
		}
		else
			d -= t;
		pdf *= (1 - prob);
	}

	return accum;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	Output.GetDimensions(dim.x, dim.y);

	StartRandomSeedForRay(dim, 1, DTid.xy, 0, PassCount);

	float4 ndcP = float4(2 * (DTid.xy + 0.5) / dim - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 D = normalize(viewT.xyz - viewP.xyz);
	float3 O = viewP.xyz;

	float3 acc = Pathtrace(O, D);

	Accumulation[DTid.xy] += acc;
	Output[DTid.xy] = Accumulation[DTid.xy] / (PassCount + 1);
}

