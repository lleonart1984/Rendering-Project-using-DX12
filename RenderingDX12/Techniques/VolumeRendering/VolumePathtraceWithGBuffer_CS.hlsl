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

RWTexture2D<float4> Radiance		: register(u0); // radiance XYZ, density = W
RWTexture2D<float4> Positions		: register(u1); //position XYZ position pdf=W
RWTexture2D<float4> Directions		: register(u2); //direction XYZ direction pdf=W
RWTexture2D<float3> Gradient		: register(u3); // density gradient in scattered position

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

float3 SampleSkybox2(float3 L) {
	float3 seaFactor = 1;
	if (L.y <= 0) // Sea
	{
		L = reflect(L, float3(0, 1, 0)); // Reflect sky
		seaFactor = float3(0.3, 0.5, 0.8);
	}
	float3 H = float3(0, 0.1, 0);

	float3 Lr = normalize(LightDirection + H * 1);
	float3 Lg = normalize(LightDirection + H * 10);
	float3 Lb = normalize(LightDirection + H * 100);

	float3 cosTheta = abs(float3(dot(L, Lr), dot(L, Lg), dot(L, Lb)));

	float3 BR_T = 3.0 / (16) * BR * (1 + cosTheta * cosTheta);
	float3 BM_T = BM / (4) * (1 - GC) / pow(1 + GC * GC + 2 * GC * cosTheta, 1.5);

	// Sky
//return LightIntensity * 0.6 * (BR_T + BM_T) / (BR + BM) * seaFactor;
	float azim = L.y;
	return lerp(float3(0.0, 0.0, 0.1), float3(0.1, 0.1, 0.4), 0.5 - azim * 0.5);
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

float SampleDensity(float3 bMin, float3 bMax, float3 P) {
	float3 tSamplePosition = (P - bMin) / (bMax - bMin);
	return Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;
}

bool NextDistance(float3 bMin, float3 bMax, inout float3 P, float3 D, float exitDistance, out float density, out float3 tSamplePosition, out float t, out float pdf) {
	pdf = 1.0;
	density = 0;
	tSamplePosition = (P - bMin) / (bMax - bMin);
	while (true) {
		t = -log(max(0.000000001, 1 - random())) / Density; // Using Density as majorant
		P += t * D;

		if (t >= exitDistance)
			return false;

		tSamplePosition = (P - bMin) / (bMax - bMin);
		density = Data.SampleGrad(VolumeSampler, tSamplePosition, 0, 0) * Density;

		float nullProb = Density - density;

		float prob = 1 - nullProb / Density;

		if (random() < prob) // scattering
			return true;
		
		pdf *= (1 - density / Density);

		exitDistance -= t;
	}
} 

// Receives a box, and a position inside the box to get a flux sample in a specific direction
float3 Pathtrace(float3 bMin, float3 bMax, float3 P, float3 D) {
	float3 accum = 0;

	// Compute volume exit distance
	float d = 1000;
	BoxIntersect(bMin, bMax, P, D, d);

	while (true) {
		float t, tpdf;
		float3 tSamplePosition;
		float density;
		if (!NextDistance(bMin, bMax, P, D, d, density, tSamplePosition, t, tpdf))
			return SampleSkybox(D) + accum;

		//if (any(isnan(P)) || any(isnan(D)))
		//	return 0;

		// next-estimation in pathtracing
		float3 lightPosition = mul(float4(tSamplePosition, 1), FromVolToAcc).xyz;
		float toLightDensity = Light.SampleGrad(LightSampler, lightPosition, 0, 0) * Density;
		accum += exp(-toLightDensity) * LightIntensity * EvalPhase(D, LightDirection);

		// Scatters to a new direction
		float scatterPdf;
		float3 scatterD;
		GeneratePhase(D, scatterD, scatterPdf); // this scatter function is importance sampled
		D = scatterD;

		// Compute next volume exit distance
		d = 1000;
		BoxIntersect(bMin, bMax, P, D, d);
	}
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	Radiance.GetDimensions(dim.x, dim.y);

	StartRandomSeedForRay(dim, 1, DTid.xy, 0, PassCount);

	float4 ndcP = float4(2 * (DTid.xy + 0.5) / dim - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 D = normalize(viewT.xyz - viewP.xyz);
	float3 P = viewP.xyz;

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	Radiance[DTid.xy] = float4(SampleSkybox(D), 0);
	Positions[DTid.xy] = float4(D, 0);
	Directions[DTid.xy] = float4(D, 1);
	Gradient[DTid.xy] = 0;

	// If not hit the volume box return skybox
	float tMin = 0, tMax = 1000;
	if (!BoxIntersect(bMin, bMax, P, D, tMin, tMax))
		return;

	P += D * tMin;

	// If first traverse doesnt hit volume densities return skybox
	float t, tpdf;
	float3 tSamplePosition;
	float density;
	
	if(!NextDistance(bMin, bMax, P, D, tMax - tMin, density, tSamplePosition, t, tpdf))
		return;

	// Compute Gradient at P
	float3 dx = P + float3(0.01, 0, 0);
	float3 dy = P + float3(0, 0.01, 0);
	float3 dz = P + float3(0, 0, 0.01);

	float3 gradient = float3(
		SampleDensity(bMin, bMax, dx) - density,
		SampleDensity(bMin, bMax, dy) - density,
		SampleDensity(bMin, bMax, dz) - density
		);

	Gradient[DTid.xy] = 0;// any(gradient) ? normalize(gradient / Density) : 0;
	Positions[DTid.xy] = float4(P, tpdf);
	// Scatters to a new direction
	float scatterPdf;
	float3 scatterD;
	GeneratePhase(D, scatterD, scatterPdf); // this scatter function is importance sampled
	D = scatterD;
	Directions[DTid.xy] = float4(D, scatterPdf);

	float3 acc = Pathtrace(bMin, bMax, P, D);
	Radiance[DTid.xy] = float4(acc, density);
}

