#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

// Photons
StructuredBuffer<Photon> Photons		: register(t0);
// Morton indice for each photon. Used as key to sort.
StructuredBuffer<int> Indices			: register(t1);

RWStructuredBuffer<int> Permutation		: register(u0);

cbuffer BitonicStage : register(b0) {
	int len;
	int dif;
};

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	int mask = dif - 1;

	int i = (index & mask) | ((index & ~mask) << 1);
	int next = i | dif;

	bool dec = Indices[Permutation[i]] > Indices[Permutation[next]];
	bool shouldDec = (i & len) != 0;
	bool swap = ((!shouldDec && dec) || (shouldDec && !dec));

	if (swap) {
		int temp = Permutation[i];
		Permutation[i] = Permutation[next];
		Permutation[next] = temp;
	}
}