#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

#ifndef ADAPTIVE_STRATEGY
#define ADAPTIVE_STRATEGY 0
#endif

StructuredBuffer<Photon> Photons				: register (t0);
StructuredBuffer<int> Morton					: register (t1);
StructuredBuffer<float> RadiiFactor				: register (t2);
StructuredBuffer<int> Permutation				: register (t3);

RWStructuredBuffer<float> Radii					: register (u0);
RWStructuredBuffer<Photon> SortedPhotons		: register (u1);

static const int bufferSize = PHOTON_DIMENSION * PHOTON_DIMENSION;

float MortonEstimator(in Photon currentPhoton, int index, float maximumRadius) {

	int l = index - 1;
	int r = index - 1;

	int currentMorton = Morton[Permutation[index]];

	for (int i = 0; i < 10; i++) {
		int currentMask = ~((1 << (i * 3)) - 1);
		int currentBlock = currentMorton & currentMask;
		float mortonBlockRadius = (1 << i) / 1024.0;

		// expand l and r considering all photons inside current block (currentMask)
		int inc = 1;
		do
		{
			l -= inc;
			//inc <<= 1;
		} while (l >= 0 && ((Morton[Permutation[l]] & currentMask) == currentBlock));
		inc = 1;
		do
		{
			r += inc;
			//inc <<= 1;
		} while (r < bufferSize && ((Morton[Permutation[r]] & currentMask) == currentBlock));
		
		if ((r - l) >= DESIRED_PHOTONS * 3.14/ 4 ) // if photons inside are sufficient then you get the desired radius
		{
			//return sqrt(mortonBlockRadius * mortonBlockRadius * DESIRED_PHOTONS / (r - l))* 4 / 3.14;
			return min(maximumRadius, mortonBlockRadius);
		}

		if (mortonBlockRadius >= maximumRadius)
			return maximumRadius;
	}
	return maximumRadius;
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int photonIdx = DTid.x;

	Photon currentPhoton = Photons[Permutation[photonIdx]];
	float radius = 0;

	if (any(currentPhoton.Intensity))
	{
#if ADAPTIVE_STRATEGY == 1
		//radius = clamp(MortonEstimator(currentPhoton, photonIdx, clamp(RadiiFactor[Permutation[photonIdx]], 1, 8) * PHOTON_RADIUS), 0.0001, PHOTON_RADIUS * 8);
		//radius = clamp(MortonEstimator(currentPhoton, photonIdx, PHOTON_RADIUS), PHOTON_RADIUS * 0.001, PHOTON_RADIUS);
		radius = MortonEstimator(currentPhoton, photonIdx, PHOTON_RADIUS);
#else
		radius = PHOTON_RADIUS;
#endif
	}

	Radii[photonIdx] = max(0.00001, radius);
	SortedPhotons[photonIdx] = currentPhoton;
}