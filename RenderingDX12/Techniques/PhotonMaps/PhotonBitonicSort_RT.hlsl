#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"

// Photons
RWStructuredBuffer<Photon> Photons		: register (u0);
// Morton indice for each photon. Used as key to sort.
RWStructuredBuffer<int> Indices			: register(u1);

cbuffer BitonicStage : register(b0) {
	int len;
	int dif;
};

[shader("raygeneration")]
void Main()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int index = raysIndex.y * raysDimensions.x + raysIndex.x;

	int mask = dif - 1;

	int i = (index & mask) | ((index & ~mask) << 1);
	int next = i | dif;

	bool dec = Indices[i] > Indices[next];
	bool shouldDec = (i & len) != 0;
	bool swap = ((!shouldDec && dec) || (shouldDec && !dec));

	if (swap) {
		Photon p = Photons[i];
		Photons[i] = Photons[next];
		Photons[next] = p;

		int temp = Indices[i];
		Indices[i] = Indices[next];
		Indices[next] = temp;
	}
}