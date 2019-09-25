#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Photon> Photons		: register (t0);

RWStructuredBuffer<int> HeadBuffer		: register (u0);
RWStructuredBuffer<int> NextBuffer		: register (u1);

#include "CommonHashing_RT.h"

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.y * PHOTON_DIMENSION + DTid.x;

	if (any(Photons[index].Intensity)) {
		int3 cell = FromPositionToCell(Photons[index].Position);
		int entry = GetHashIndex(cell);
		InterlockedExchange(HeadBuffer[entry], index, NextBuffer[index]);
	}
}
