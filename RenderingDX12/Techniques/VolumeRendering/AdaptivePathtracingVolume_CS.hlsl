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

Texture3D<float> Data		: register(t0);
Texture3D<float> Light		: register(t1);
Texture3D<float2> Sums		: register(t2);

sampler VolumeSampler	: register(s0);
sampler LightSampler	: register(s1);

RWTexture2D<float3> Accumulation	: register(u0);
RWTexture2D<float3> Output			: register(u1);

//#include "PhaseIso.h"
#include "PhaseAniso.h"

float3 SampleSkybox(float3 L) {
	float azim = L.y;
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

float2 TotalSum(int3 bMin, int3 bMax) {
	int3 c000 = int3(bMin.x, bMin.y, bMin.z);
	int3 c001 = int3(bMin.x, bMin.y, bMax.z);
	int3 c010 = int3(bMin.x, bMax.y, bMin.z);
	int3 c011 = int3(bMin.x, bMax.y, bMax.z);
	int3 c100 = int3(bMax.x, bMin.y, bMin.z);
	int3 c101 = int3(bMax.x, bMin.y, bMax.z);
	int3 c110 = int3(bMax.x, bMax.y, bMin.z);
	int3 c111 = int3(bMax.x, bMax.y, bMax.z);

	return
		  Sums[c111]
		- Sums[c011]
		- Sums[c101]
		- Sums[c110]
		+ Sums[c010]
		+ Sums[c100]
		+ Sums[c001]
		- Sums[c000];
}

float EstimateMajorant(float3 bMin, float3 bMax, float3 P, float3 D, float tMax) {
	float3 o = (P - bMin) / (bMax - bMin);
	float3 d = (P + D * tMax - bMin) / (bMax - bMin);

	float3 rMin = min(o, d);
	float3 rMax = max(o, d);

	int3 start = rMin * Dimensions - 1;
	int3 end = min(Dimensions - 1, rMax * Dimensions);

	int volume = (end.x - start.x) * (end.y - start.y) * (end.z - start.z);

	float2 sums = TotalSum(start, end);

	float mn = sums.x / volume; // mean value

	float var = abs(sums.y / volume - mn * mn);

	float sd = sqrt(var);

	float sqrtVol = sqrt(volume);

	float minValueForMax = mn + sd / sqrtVol;
	float maxValueForMax = mn + sd * (sqrtVol - 1);

	return maxValueForMax;// mn + sd;
}

void BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMax)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = abs(D2) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
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

	int N = 10000;
	float phongNorm = (N + 2) / (2 * pi);

	float d = tMax - tMin;

	int bounces = 0;

	while (true)
		//while (bounces < Absortion*200) 
	{
		float majorant = Density * max(0.001, min(1, EstimateMajorant(bMin, bMax, P, D, d)));

		float t = -log(1 - random()) / majorant; // Using estimated majorant
		float tpdf = exp(-t * majorant) * majorant;

		if (t >= d)
			return
			SampleSkybox(D)
			+ accum;// +pow(max(0, dot(D, LightDirection)), N) * phongNorm * LightIntensity;

		P += t * D;

		float3 tSamplePosition = (P - bMin) / (bMax - bMin);
		float densityt = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

		float nullProb = majorant - densityt;

		float prob = 1 - nullProb / majorant;

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
		}
		else
			d -= t;
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

	//if (PassCount < 16)
	{
		Accumulation[DTid.xy] = (Accumulation[DTid.xy] * PassCount + acc) / (PassCount + 1);
		Output[DTid.xy] = Accumulation[DTid.xy];
	}
}

