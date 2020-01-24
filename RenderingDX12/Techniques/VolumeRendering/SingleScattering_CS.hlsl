#include "../CommonGI/Parameters.h"

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

Texture3D<float> Data : register(t0);
Texture3D<float> Light : register(t1);

sampler VolumeSampler : register(s0);

RWTexture2D<float3> Output			: register(u0);

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

float Phase(float3 i, float3 o) {
	return 3.0 / 4.0 * (1 + dot(i, o) * dot(i, o));
}

void RayTraversal(float3 beg, float3 end, float3 bMin, float3 bMax, out float3 total)
{
	float3 D = end - beg;
	float totalDistance = length(D);
	D /= totalDistance;

	float3 V = -D;
	float3 L = normalize(LightDirection);

	float step = 0.5 / max(Dimensions.x, max(Dimensions.y, Dimensions.z)); // half of a voxel size

	total = 0;

	float intSigmaT = 0;

	float t = 0;
	while (t < totalDistance)
	{
		float3 samplePosition = (beg + D * t - bMin)/(bMax - bMin);

		// Do something with the Voxel
		float density = Data.SampleGrad(VolumeSampler, samplePosition, 0, 0)* Density;

		float3 lightPosition = mul(float4(samplePosition, 1), FromVolToAcc).xyz;
		float3 toLightDensity = Light.SampleGrad(VolumeSampler, lightPosition, 0, 0) * Density;

		float sigmaA = density * Absortion;
		float sigmaS = density * (1 - Absortion); // scattering
		float sigmaT = sigmaA + sigmaS;

		total += step * exp(-(intSigmaT + step * 0.5 * sigmaT));// *sigmaS* exp(-toLightDensity)* LightIntensity / (4 * 3.14159);
		intSigmaT += step * sigmaT;

		// Move to next step
		t += step;
	}

	//total += exp(-intSigmaT) * float3(0.5, 0.6, 1);
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
	float3 D = normalize(viewT.xyz - viewP.xyz);

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float3 total = 0;
	float importance = 1;
	int Samples = 1000;
	float tMin = 0;
	float tMax = 1000;
	if (BoxIntersect(bMin, bMax, O, D, tMin, tMax)) {
		RayTraversal(O + D * tMin, O + D * tMax, bMin, bMax, total);
	}
	else total = float3(0.5, 0.6, 1);
	Output[DTid.xy] = total;
}

