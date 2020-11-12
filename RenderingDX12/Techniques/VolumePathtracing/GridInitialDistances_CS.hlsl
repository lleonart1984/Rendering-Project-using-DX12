#include "../CommonGI/Definitions.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<int> OB : register(t1); // Object buffers
StructuredBuffer<float4x4> Transforms : register(t2); // World transforms
StructuredBuffer<int> TriangleIndices : register(t3); // Linked list values (references to the triangles)
Texture3D<int> Head : register(t4); // Per cell linked list head
StructuredBuffer<int> Next : register(t5); // Per linked lists next references....

RWTexture3D<float> DistanceField : register(u0);

cbuffer GridInfo : register(b0) {
	int Size;
	float3 Min;
	float3 Max;
}

float Distance(int triangleIndex, float3 N, float3 P) {
	float4x4 world = Transforms[OB[triangleIndex * 3]];

	float3 c1 = mul(vertices[triangleIndex * 3 + 0].P, world);
	float3 c2 = mul(vertices[triangleIndex * 3 + 1].P, world);
	float3 c3 = mul(vertices[triangleIndex * 3 + 2].P, world);

	return min(dot(c1 - P, N), min(dot(c2 - P, N), dot(c3 - P, N)));
}

float DistancePoint2Triangle(float3 P, float3 V0, float3 V1, float3 V2) {

}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int3 currentCell = int3(DTid.x % Size, DTid.x / Size % Size, DTid.x / (Size * Size));

	if (Head[currentCell] != -1) // not empty cell
		return; // leave distance equals 0

	float3 cellCenter = Min + (Max - Min) * (currentCell + 0.5) / Size;

	float minDistance = 1000;
	for (int bz = -1; bz <= 1; bz++)
		for (int by = -1; by <= 1; by++)
			for (int bx = -1; bx <= 1; bx++)
			{
				int3 adjCell = clamp(int3(bx, by, bz) + currentCell, 0, Size - 1);
				
				int currentTriangle = Head[adjCell];
				while (currentTriangle != -1) {

					float3 P = cellCenter + float3(bx, by, bz) * 0.5;
					float3 N = normalize(float3(bx, by, bz));

					minDistance = min(minDistance, Distance(currentTriangle, N, P));

					currentTriangle = Next[currentTriangle];
				}
			}

	DistanceField[currentCell] = minDistance;
}
