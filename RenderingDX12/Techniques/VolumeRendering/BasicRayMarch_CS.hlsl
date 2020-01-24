#include "../CommonGI/Parameters.h"

cbuffer Camera : register(b0) {
	row_major matrix FromProjectionToWorld;
};

cbuffer Volume : register(b1) {
	int3 Dimensions;
	float Density;
	float Absortion;
}

Texture3D<float> Data : register(t0);

sampler VolumeSampler : register(s0);

RWTexture2D<float3> Output : register(u0);

void GetVolumeBox(out float3 minim, out float3 maxim) {
	float maxC = max(Dimensions.x, max(Dimensions.y, Dimensions.z));
	minim = -Dimensions * 0.5 / maxC;
	maxim = -minim;
}

bool BoxIntersect(float3 bMin, float3 bMax, float3 P, float3 D, inout float tMin, inout float tMax)
{
	float2x3 C = float2x3(bMin - P, bMax - P);
	float2x3 D2 = float2x3(D, D);
	float2x3 T = D2 == 0 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
	tMin = max(max(min(T._m00, T._m10), min(T._m01, T._m11)), min(T._m02, T._m12));
	tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
	if (tMax < tMin || tMax < 0) {
		return false;
	}

	tMin = max(0, tMin);
	return true;
}

float SampleVolume(float3 cell) {
	return Data.SampleGrad(VolumeSampler, cell / Dimensions, 0, 0);
}

void RayTraversal(float3 beg, float3 end, float3 bMin, float3 bMax, out float3 total)
{
	float3 P = (beg - bMin) * Dimensions / (bMax - bMin);
	float3 H = (end - bMin) * Dimensions / (bMax - bMin);

	float3 D = H - P;

	float3 step = D == 0 ? 1 : 1 / abs(D);

	int3 voxelBorder = (D > 0) + int3(P);

	float3 alpha = D == 0 ? 1 : (voxelBorder - P) / D;

	int3 voxelInc = D > 0 ? 1 : -1;

	float currentAlpha = 0;
	int3 currentVoxel = int3(P);

	float alphaToDistance = length(end - beg);

	total = 0;

	float intSigmaT = 0;

	float t = 1;
	while (currentAlpha < t)
	{
		// Decide next step
		int3 selection = alpha.x <= min(alpha.y, alpha.z) ? int3(1, 0, 0) : int3(0, alpha.y <= alpha.z, alpha.y > alpha.z);
		float nextAlpha = min(t, dot(selection, alpha));
		float stepdistance = (nextAlpha - currentAlpha) * alphaToDistance;
		float distance = nextAlpha * alphaToDistance;

		// Do something with the Voxel
		float density = SampleVolume(lerp(P, H, lerp(currentAlpha, nextAlpha, 0.5))) * 10;

		float sigmaS = density * Absortion; // scattering
		float sigmaT = density * Absortion;

		intSigmaT += stepdistance * sigmaT;
		total += exp(-intSigmaT) * (sigmaS) * stepdistance;


		// Move to next voxel
		alpha += selection * step;
		currentVoxel += selection * voxelInc;
		currentAlpha = nextAlpha;
	}
}

void RayTraversal2(float3 beg, float3 end, float3 bMin, float3 bMax, out float3 total)
{
	float3 P = (beg - bMin) * Dimensions / (bMax - bMin);
	float3 H = (end - bMin) * Dimensions / (bMax - bMin);

	float3 D = H - P;

	float step = 0.5 / max(Dimensions.x, max(Dimensions.y, Dimensions.z)); // half of a voxel size

	float currentAlpha = 0;
	float alphaToDistance = length(end - beg);

	float alphaStep = step / alphaToDistance;

	total = 0;

	float intSigmaT = 0;

	float t = 1;
	while (currentAlpha < t)
	{
		float nextAlpha = currentAlpha + alphaStep;
		float stepdistance = alphaStep * alphaToDistance;
		float distance = nextAlpha * alphaToDistance;

		// Do something with the Voxel
		float density = SampleVolume(lerp(P, H, currentAlpha)) * Density;

		float sigmaA = density * Absortion;
		float sigmaS = density * (1 - Absortion); // scattering
		float sigmaT = sigmaA + sigmaS;

		intSigmaT += stepdistance * sigmaT;
		total += exp(-intSigmaT) * (sigmaS)*stepdistance;

		// Move to next step
		currentAlpha = nextAlpha;
	}

	total += exp(-intSigmaT) * float3(0.5, 0.6, 1);
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	Output.GetDimensions(dim.x, dim.y);

	float4 ndcP = float4(2 * (DTid.xy + 0.5) / dim - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT - viewP);

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float3 total = 0;
	float importance = 1;
	int Samples = 1000;
	float tMin = 0;
	float tMax = 1000;
	if (BoxIntersect(bMin, bMax, O, D, tMin, tMax)) {
		RayTraversal2(O + D * tMin, O + D * tMax, bMin, bMax, total);
	}
	Output[DTid.xy] = total;
}

