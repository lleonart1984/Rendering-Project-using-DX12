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
Texture3D<float> Majorants	: register(t2);

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

void BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMax)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = abs(D2) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
}

bool NextDistance(float3 bMin, float3 bMax, inout float3 P, float3 D, float exitDistance, out float density, out float3 tSamplePosition, out float acct, out float pdf) {
	
	pdf = 1.0;
	density = 0;
	tSamplePosition = (P - bMin) / (bMax - bMin);
	int3 cell = ((int3)(tSamplePosition * Dimensions)) / MAJORANT_VOLUMEN_CELL_DIM; // MAJORANT_VOLUMEN_CELL_DIM^3 cell size

	float cellSize = (bMax.x - bMin.x) * MAJORANT_VOLUMEN_CELL_DIM / Dimensions.x;

	float3 step = D == 0 ? 1000 : cellSize / abs(D);

	int3 voxelBorder = (D > 0) + cell;

	float3 alpha = D == 0 ? 1000 : (voxelBorder * cellSize - (P - bMin)) / D;

	int3 voxelInc = D > 0 ? 1 : -1;

	acct = 0;
	while (true) {

		float majorant = max(0.001, Majorants[cell] * Density);

		float t = -log(1 - random()) / majorant; // Using Majorants grid value as majorant
		acct += t;
		P += t * D;

		int3 selection = alpha.x <= min(alpha.y, alpha.z) ? int3(1, 0, 0) : int3(0, alpha.y <= alpha.z, alpha.y > alpha.z);
		float nextAlpha = dot(selection, alpha);

		if (acct >= nextAlpha) // free passed through current cell
		{
			float backDistance = acct - nextAlpha;
			acct = nextAlpha;
			P -= backDistance * D;

			alpha += selection * step; // update alphas
			cell += selection * voxelInc; // update cell
		}
		else
		{
			tSamplePosition = (P - bMin) / (bMax - bMin);
			density = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

			float nullProb = majorant - density;

			float prob = 1 - nullProb / majorant;

			if (random() < prob) // scattering
				return true;
			
			pdf *= (1 - density / majorant);
		}

		if (nextAlpha >= exitDistance)
			return false;
	}
}

// Receives a box, and a position inside the box to get a flux sample in a specific direction
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

	while (true)
		//while (bounces < Absortion*200) 
	{

		float t, tpdf;
		float3 tSamplePosition;
		float density;
		if (!NextDistance(bMin, bMax, P, D, d, density, tSamplePosition, t, tpdf))
			return SampleSkybox(D) + accum;

		float3 lightPosition = mul(float4(tSamplePosition, 1), FromVolToAcc).xyz;
		float toLightDensity = Light.SampleGrad(LightSampler, lightPosition, 0, 0) * Density;

		accum += exp(-toLightDensity) * LightIntensity * EvalPhase(D, LightDirection);

		float scatterPdf;
		float3 scatterD;
		GeneratePhase(D, scatterD, scatterPdf);

		D = scatterD;

		d = 1000;
		BoxIntersect(bMin, bMax, P, D, d);

		bounces++;
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

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	float3 acc = Pathtrace(O, D);

	//if (PassCount < 1000) {
	Accumulation[DTid.xy] = (Accumulation[DTid.xy] * PassCount + acc) / (PassCount + 1);
	Output[DTid.xy] = Accumulation[DTid.xy];
	//}
}

