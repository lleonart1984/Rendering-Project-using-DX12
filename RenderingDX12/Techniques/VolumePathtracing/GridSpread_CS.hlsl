#include "../CommonGI/Definitions.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<uint>		GridSrc : register(t0);

Texture3D<int> Head : register(t1); // Per cell linked list head (just to recognize empty cells)
RWTexture3D<float>	DistanceField  : register(u0);

cbuffer GridInfo : register(b0) {
	int Size;
	float3 Min;
	float3 Max;
}

cbuffer LevelInfo : register(b1) {
	int Level;
	int Radius;
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
				int3 adjCell = clamp(int3(bx, by, bz) * (Radius) + currentCell, 0, Size - 1);
				uint levelAtAdjCell = GetValueAt(adjCell);
				allEmpty &= levelAtAdjCell == Level;
			}
	
	if (allEmpty) // can be spread
		SetValueAt(currentCell, Level + 1);
	else
		SetValueAt(currentCell, GetValueAt(currentCell));
}
