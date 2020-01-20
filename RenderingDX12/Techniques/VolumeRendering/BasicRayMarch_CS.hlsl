#include "../CommonGI/Parameters.h"

cbuffer Camera : register(b0) {
	row_major matrix FromProjectionToWorld;
};

cbuffer Volume : register(b1) {
	int Width;
	int Height;
	int Slices;
	float Absortion;
}

StructuredBuffer<float> Data : register(t0);

RWTexture2D<float3> Output : register(u0);

void GetVolumeBox(out float3 minim, out float3 maxim) {
	int3 sizes = int3(Width, Height, Slices);
	float maxC = max(Width, max(Height, Slices));
	minim = -sizes * 0.5 / maxC;
	maxim = -minim;
}

bool FromPositionToVolume(float3 P, float3 minCoord, float3 maxCoord, out int3 cell) {
	int3 sizes = int3(Width, Height, Slices);
	cell = (P - minCoord) * sizes / (maxCoord - minCoord);
	return all(cell >= 0) && all(cell < sizes);
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

void RayTraversal(float3 beg, float3 end, float3 bMin, float3 bMax, out float3 total)
{
	int3 sizes = int3(Width, Height, Slices);
	
	float3 P = (beg - bMin) * sizes / (bMax - bMin);
	float3 H = (end - bMin) * sizes / (bMax - bMin);

	float3 D = H - P;

	float3 step = D == 0 ? 1 : 1 / abs(D);

	int3 voxelBorder = (D > 0) + int3(P);

	float3 alpha = D == 0 ? 1 : (voxelBorder - P) / D;

	int3 voxelInc = D > 0 ? 1 : -1;

	float currentAlpha = 0;
	int3 currentVoxel = int3(P);

	float alphaToDistance = length(end - beg);

	total = 0;

	float3 importance = 1;

	float t = 1;
	while (currentAlpha < t)
	{
		// Decide next step
		int3 selection = alpha.x <= min(alpha.y, alpha.z) ? int3(1, 0, 0) : int3(0, alpha.y <= alpha.z, alpha.y > alpha.z);
		float nextAlpha = min(t, dot(selection, alpha));
		float distance = (nextAlpha - currentAlpha) * alphaToDistance;

		// Do something with the Voxel
		int index = currentVoxel.x * Height * Slices + currentVoxel.y * Slices + currentVoxel.z;
		float density = saturate(Data[index] * 100 * Absortion * distance);
		total += density * importance;
		importance *= (1 - density);

		// Move to next voxel
		alpha += selection * step;
		currentVoxel += selection * voxelInc;
		currentAlpha = nextAlpha;
	}
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

	float total = 0;
	float importance = 1;
	int Samples = 1000;
	float tMin = 0;
	float tMax = 1000;
	if (BoxIntersect(bMin, bMax, O, D, tMin, tMax)) {
		RayTraversal(O + D * tMin, O + D * tMax, bMin, bMax, total);
	}
	Output[DTid.xy] = total;
}

/*

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
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

	float total = 0;
	float importance = 1;
	int Samples = 1000;
	float tMin = 0;
	float tMax = 1000;
	BoxIntersect(bMin, bMax, O, D, tMin, tMax);

	for (int i = 0; i < Samples; i++)
	{
		float alpha = tMin + (tMax - tMin) * i / Samples;
		float3 P = O + D * alpha;

		int3 cell;
		if (FromPositionToVolume(P, bMin, bMax, cell))
		{
			int index = cell.x * Height * Slices + cell.y * Slices + cell.z;

			float density = saturate(Data[index] * Absortion);

			total += density * importance;
			importance *= (1 - density);
		}
	}
	Output[DTid.xy] = total;
}

*/