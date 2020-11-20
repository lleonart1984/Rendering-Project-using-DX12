#include "../CommonGI/Definitions.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<int> OB : register(t1); // Object buffers
StructuredBuffer<float4x4> Transforms : register(t2); // World transforms

RWStructuredBuffer<int> TriangleIndices : register(u0); // Linked list values (references to the triangles)
RWTexture3D<int> Head : register(u1); // Per cell linked list head
RWStructuredBuffer<int> Next : register(u2); // Per linked lists next references....
RWStructuredBuffer<int> Malloc : register(u3); // incrementer buffer

cbuffer GridInfo : register(b0) {
	int Size;
	float3 Min;
	float3 Max;
}

float3 FromPositionToCell(float3 P, float4x4 world) {
	return (mul(float4(P, 1), world).xyz - Min) * Size / (Max - Min);
}

void ClampPositive(inout float3 a, inout float3 b, float3 P, float3 N) {
	float3 d = b - a;
	float alpha = clamp(dot(a - P, N) / dot(d, N), 0, 1);
	a += d * max(0, alpha);
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint NumberOfVertices, stride;
	vertices.GetDimensions(NumberOfVertices, stride);

	if (DTid.x >= NumberOfVertices / 3)
		return;

	float4x4 world = Transforms[OB[DTid.x * 3]];

	float3 c1 = FromPositionToCell(vertices[DTid.x * 3 + 0].P, world);
	float3 c2 = FromPositionToCell(vertices[DTid.x * 3 + 1].P, world);
	float3 c3 = FromPositionToCell(vertices[DTid.x * 3 + 2].P, world);

	float3 P = c1;
	float3 N = cross(c3 - c1, c2 - c1);

	int3 maxCell = max(c1, max(c2, c3));// +0.00001;
	int3 minCell = min(c1, min(c2, c3));// -0.00001;

	float2x4 evals = float2x4(
		dot(minCell + float3(0, 0, 0) - P, N), dot(minCell + float3(1, 0, 0) - P, N), dot(minCell + float3(0, 1, 0) - P, N), dot(minCell + float3(1, 1, 0) - P, N),
		dot(minCell + float3(0, 0, 1) - P, N), dot(minCell + float3(1, 0, 1) - P, N), dot(minCell + float3(0, 1, 1) - P, N), dot(minCell + float3(1, 1, 1) - P, N)
		);

	for (int cz = minCell.z; cz <= maxCell.z; cz++)
		for (int cy = minCell.y; cy <= maxCell.y; cy++)
			for (int cx = minCell.x; cx <= maxCell.x; cx++)
			{
				int3 currentCell = int3(cx, cy, cz);
				float2x4 currentEvals = evals + dot(N, currentCell - minCell);
				if (!all(currentEvals <= 0) && !all(currentEvals >= 0)) // cell intersects triangle
				{
					int currentReference;
					InterlockedAdd(Malloc[0], 1, currentReference);
					TriangleIndices[currentReference] = DTid.x;
					InterlockedExchange(Head[currentCell], currentReference, Next[currentReference]);
				}
			}
}