#include "../CommonGI/Definitions.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<int> OB : register(t1); // Object buffers
StructuredBuffer<float4x4> Transforms : register(t2); // World transforms

RWStructuredBuffer<uint> Grid : register(u0);

cbuffer GridInfo : register(b0) {
	int Size;
	float3 Min;
	float3 Max;
}

float3 FromPositionToCell(float3 P, float4x4 world) {
	return (mul(float4(P, 1), world).xyz - Min) * Size / (Max - Min);
}

int split3(int value) {

	int ans = value & 0x3FF; // allow 10 bits only.

	ans = (ans | ans << 16) & 0xff0000ff;
	// poniendo 8 ceros entre cada grupo de 4 bits.
	// shift left 8 bits y despues hacer OR entre ans y 0001000000001111000000001111000000001111000000001111000000000000
	ans = (ans | ans << 8) & 0x0f00f00f;
	// poniendo 4 ceros entre cada grupo de 2 bits.
	// shift left 4 bits y despues hacer OR entre ans y 0001000011000011000011000011000011000011000011000011000011000011
	ans = (ans | ans << 4) & 0xc30c30c3;
	// poniendo 2 ceros entre cada bit.
	// shift left 2 bits y despues hacer OR entre ans y 0001001001001001001001001001001001001001001001001001001001001001
	ans = (ans | ans << 2) & 0x49249249;
	return ans;
}

int morton(int3 pos) {
	return split3(pos.x) | (split3(pos.y) << 1) | (split3(pos.z) << 2);
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint NumberOfVertices, stride;
	vertices.GetDimensions(NumberOfVertices, stride);
	
	if (DTid.x >= NumberOfVertices / 3)
		return;

	float4x4 world = Transforms[OB[DTid.x * 3]];

	float3 c1 = FromPositionToCell(vertices[DTid.x * 3 + 0].P, world);
	float3 c2 = FromPositionToCell(vertices[DTid.x * 3 + 1].P, world);
	float3 c3 = FromPositionToCell(vertices[DTid.x * 3 + 2].P, world);

	int3 maxCell = max(c1, max(c2, c3));// +0.00001;
	int3 minCell = min(c1, min(c2, c3));// -0.00001;

	for (int bz = minCell.z / 4; bz <= maxCell.z / 4; bz++)
		for (int by = minCell.y / 4; by <= maxCell.y / 4; by++)
			for (int bx = minCell.x / 4; bx <= maxCell.x / 4; bx++)
			{
				int3 start = max(minCell - int3(bx, by, bz) * 4, 0);
				int3 end = min(maxCell - int3(bx, by, bz) * 4, 3);
				uint2 mask = 0;
				for (int z = start.z; z <= end.z; z++)
					for (int y = start.y; y <= end.y; y++)
						for (int x = start.x; x <= end.x; x++)
						{
							int cmp = z / 2;
							int bit = (z % 2) * 16 + y * 4 + x;
							mask[cmp] |= 1 << (31 - bit);
						}
				int pos = morton(int3(bx, by, bz));
				InterlockedOr(Grid[pos*2+0], mask.x);
				InterlockedOr(Grid[pos*2+1], mask.y);
				//InterlockedOr(Grid[pos*2], 0xFFFFFFFF);
				//InterlockedOr(Grid[pos*2+1], 0xFFFFFFFF);
			}
}