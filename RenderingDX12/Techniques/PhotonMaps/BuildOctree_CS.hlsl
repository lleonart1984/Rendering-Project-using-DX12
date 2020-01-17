#include "../CommonGI/Parameters.h"

StructuredBuffer<int> MortonIndices		: register(t0);
StructuredBuffer<int> Permutation		: register(t1);

RWStructuredBuffer<int> LOD				: register(u0);
RWStructuredBuffer<int> Skip			: register(u1);

cbuffer Stage : register(b0) {
	int BlockLOD;
};

int GetLOD(uint m1, uint m2) {
	int lod = 9;

	while (lod >= 0 && ((m1 & (7 << (lod * 3))) == (m2 & (7 << (lod * 3)))))
		lod--;

	return lod + 1;
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (BlockLOD == 0)
	{
		LOD[DTid.x] = 0;
		Skip[DTid.x] = DTid.x + 1;
	}
	else 
	{
		//uint length, stride;
		//LOD.GetDimensions(length, stride);

		int size = 1 << BlockLOD;
		int index1 = DTid.x * size;
		int index2 = DTid.x * size + size - 1;

		if (index2 >= PHOTON_DIMENSION*PHOTON_DIMENSION)
			return;

		int pref1 = MortonIndices[Permutation[index1]];
		int pref2 = MortonIndices[Permutation[index2]];
		
		int newLOD = GetLOD(pref1, pref2);
		int newSkip = (DTid.x + 1) * size;

		/*for (int i = 1; i > 0; i--)
		{
			Skip[index1 + i] = Skip[index1 + i - 1];
			LOD[index1 + i] = LOD[index1 + i - 1];
		}*/

		Skip[index1] = newSkip;
		LOD[index1] = newLOD;
	}
}