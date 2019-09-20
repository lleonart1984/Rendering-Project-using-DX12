#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Photon> Photons		: register (t0);

RWStructuredBuffer<int> HeadBuffer		: register (u0);
RWStructuredBuffer<int> NextBuffer		: register (u1);

#include "CommonHashing_RT.h"

[shader("raygeneration")]
void Main()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int index = raysIndex.y * raysDimensions.x + raysIndex.x;

	if (any(Photons[index].Intensity)) {
		int3 cell = FromPositionToCell(Photons[index].Position);
		int entry = GetHashIndex(cell);
		InterlockedExchange(HeadBuffer[entry], index, NextBuffer[index]);
	}
}