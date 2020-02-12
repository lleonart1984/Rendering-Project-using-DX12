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

cbuffer PathtracingInfo : register(b2) {
	int PassCount;
};

Texture3D<float> Data			: register(t0);
Texture2D<float3> DirectLight	: register(t1);

sampler VolumeSampler : register(s0);

RWTexture2D<float3> Accumulation	: register(u0);
RWTexture2D<float3> Output			: register(u1);

//#include "PhaseIso.h"
#include "PhaseAniso.h"

float3 SampleSkybox(float3 L) {
	float azim = L.y;
	//return lerp(float3(0.9, 0.95, 1), float3(0.1, 0.2, 0.5), -sign(azim)*0.5 + 0.5);
	return lerp(float3(0.7, 0.8, 1), float3(0.5, 0.3, 0.2), 0.5 - azim * 0.5);
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

float3 Pathtrace(float3 P, float3 D) {

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float tMin = 0;
	float tMax = 1000;
	BoxIntersect(bMin, bMax, P, D, tMin, tMax);

	P += D * tMin; // Put P inside the volume

	int N = 1000;
	float phongNorm = (N + 2) / (4 * pi);

	float d = tMax - tMin;
	int bounces = 1;

	while (true) {

		float t = -log(1 - random()) / Density; // Using Density as majorant
		float tpdf = exp(-t * Density) * Density;

		if (t >= d)
			return SampleSkybox(D);

		P += t * D;

		float3 tSamplePosition = (P - bMin) / (bMax - bMin);
		float densityt = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

		float nullProb = Density - densityt;

		float prob = 1 - nullProb / Density;

		if (random() < prob) // scattering
		{
			float scatterPdf;
			float3 scatterD;
			GeneratePhase(D, scatterD, scatterPdf);

			D = scatterD;

			tMin = 0;
			tMax = 1000;
			BoxIntersect(bMin, bMax, P, D, tMin, tMax);

			P += D * tMin;

			d = tMax - tMin;
			bounces--;
		}
		else
		{
			d -= t;
		}
	}
	return 0;
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

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	float3 acc = Pathtrace(O, D)
		+ DirectLight[DTid.xy]
		;

	//if (PassCount < 100) 
	{
		Accumulation[DTid.xy] = (Accumulation[DTid.xy] * PassCount + acc) / (PassCount + 1);
		Output[DTid.xy] = Accumulation[DTid.xy];
	}
}

