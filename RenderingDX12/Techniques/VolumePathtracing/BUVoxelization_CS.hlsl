#include "../CommonGI/Parameters.h"

RWStructuredBuffer<uint2> Grid : register(u0);

cbuffer GridRange : register(b0) {
	int beg;
	int end;
};

uint Group(uint x) {
	x = (x | (x >> 1) | (x >> 2) | (x >> 3) | (x >> 4) | (x >> 5) | (x >> 6) | (x >> 7));
	x &= 0x01010101;
	x = ((x >> 07) | x) & 0x00030003;
	x = ((x >> 14) | x) & 0x0000000F;
	return x;
	//return (x.x << 4) | x.y;
}

uint Group2(uint2 x) {
	return Group(x.x) << 4 | Group(x.y);
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	if (DTid.x >= (end - beg) / 8)
		return; // group kernell fall outside grid range analyzed

	uint totalx = 0;
	for (int i = 0; i < 4; i++)
	{
		uint b = Group2(Grid[beg + DTid.x * 8 + i]);
		totalx |= b << (32 - (i + 1) * 8);
	}

	uint totaly = 0;
	for (int i = 0; i < 4; i++)
	{
		uint b = Group2(Grid[beg + DTid.x * 8 + 4 + i]);
		totaly |= b << (32 - (i + 1) * 8);
	}
	Grid[end + DTid.x] = uint2(totalx, totaly);
	//Grid[end + DTid.x] = totalx != 0 || totaly != 0 ?
	//	uint2(0xFFFFFFFF, 0xFFFFFFFF) : 0;
}