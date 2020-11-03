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

uint GetValueAt(int3 cell) {
	//int index = Size * (cell.y + cell.z * Size) + cell.x;
	int index = morton(cell);
	return (GridSrc[index >> 3] >> ((index & 0x7) * 4)) & 0xF;
}

void SetValueAt(int3 cell, uint value) {
	//int index = Size * (cell.y + cell.z * Size) + cell.x;
	int index = morton(cell);
	InterlockedOr(GridDst[index >> 3], (uint)(value << ((index & 0x7) * 4)));
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int3 currentCell = int3(DTid.x % Size, DTid.x / Size % Size, DTid.x / (Size * Size));

	if (Level == 0) // complement occupied 1 to 0 and empty 0 to 1
	{
		SetValueAt(currentCell, 1 - GetValueAt(currentCell));
		return;
	}

	bool allEmpty = true;
	for (int bz = -1; bz <= 1; bz++)
		for (int by = -1; by <= 1; by++)
			for (int bx = -1; bx <= 1; bx++)
			{
				int3 adjCell = clamp(int3(bx, by, bz) * (Radius)+currentCell, 0, Size - 1);
				uint levelAtAdjCell = GetValueAt(adjCell);
				allEmpty &= levelAtAdjCell == Level;
			}

	if (allEmpty) // can be spread
		SetValueAt(currentCell, Level + 1);
	else
		SetValueAt(currentCell, GetValueAt(currentCell));
}
