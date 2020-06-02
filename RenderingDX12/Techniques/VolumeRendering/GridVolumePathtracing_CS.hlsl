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

#define BR 1.0
#define BM 1.0
#define GC 0



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
	//for (int i = 1; i < 5; i++)
	col = lerp(col, BG_COLORS[1], smoothstep(BG_DISTS[0], BG_DISTS[1], L.y));
	col = lerp(col, BG_COLORS[2], smoothstep(BG_DISTS[1], BG_DISTS[2], L.y));
	col = lerp(col, BG_COLORS[3], smoothstep(BG_DISTS[2], BG_DISTS[3], L.y));
	col = lerp(col, BG_COLORS[4], smoothstep(BG_DISTS[3], BG_DISTS[4], L.y));
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
	if (tMax <= tMin || tMax <= 0.001) {
		return false;
	}
	return true;
}

void BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMax)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = abs(D2) <= 0.000001 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	tMax = max(0, min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12)));
}

bool NextDistance(float3 bMin, float3 bMax, inout float3 P, float3 D, float exitDistance, out float density, out float3 tSamplePosition, out float acct, out float pdf) {
	
	pdf = 1.0;
	density = 0;
	tSamplePosition = (P - bMin) / (bMax - bMin);
	int3 cell = ((int3)(tSamplePosition * Dimensions)) / MAJORANT_VOLUMEN_CELL_DIM; // MAJORANT_VOLUMEN_CELL_DIM^3 cell size

	float cellSize = (bMax.x - bMin.x) * MAJORANT_VOLUMEN_CELL_DIM / Dimensions.x;

	float3 step = abs(D) <= 0.001 ? 1000 : cellSize / abs(D);

	int3 voxelBorder = (D > 0) + cell;

	float3 alpha = abs(D) <= 0.001 ? 1000 : (voxelBorder * cellSize - (P - bMin)) / D;

	int3 voxelInc = D > 0 ? 1 : -1;
	
	acct = 0;
	while(true) {

		float majorant = max(0.00001, Majorants[cell] * Density);
		float t = -log(max(0.0000001, 1 - random())) / majorant; // Using Majorants grid value as majorant
		acct += t;

		int3 selection = alpha.x <= min(alpha.y, alpha.z) ? int3(1, 0, 0) : int3(0, alpha.y <= alpha.z, alpha.y > alpha.z);
		float nextAlpha = dot(selection, alpha);

		if (acct >= nextAlpha) // go outside cell
		{ // increase cell
			acct = nextAlpha; // move to cell border
			alpha += selection * step; // update alphas
			cell += selection * voxelInc; // update cell
			if (nextAlpha >= exitDistance) // across all volume
				return false;
		}
		else
		{ // probable to scatter
			float3 sampleP = P + D * acct;

			tSamplePosition = (sampleP - bMin) / (bMax - bMin);
			density = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

			float prob = density / majorant;
			pdf *= (1 - prob);
			if (random() < prob)
			{
				P += D * acct;
				return true;
			}
		}
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

	while (true)
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
	Output[DTid.xy] = float4(Accumulation[DTid.xy].xyz, 1);
	//}
}

