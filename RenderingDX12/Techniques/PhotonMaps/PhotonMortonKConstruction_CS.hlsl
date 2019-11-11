#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

#ifndef ADAPTIVE_STRATEGY
#define ADAPTIVE_STRATEGY 0
#endif

struct AABB {
	float3 minimum;
	float3 maximum;
};

StructuredBuffer<Photon> Photons				: register (t0);
StructuredBuffer<int> Morton					: register (t1);
StructuredBuffer<float> RadiiFactor				: register (t2);
StructuredBuffer<int> Permutation				: register (t3);

RWStructuredBuffer<AABB> PhotonAABBs			: register (u0);
RWStructuredBuffer<float> radii					: register (u1);
RWStructuredBuffer<Photon> AllocatedPhotons		: register (u2);

static const int bufferSize = PHOTON_DIMENSION * PHOTON_DIMENSION - 1;

float MortonEstimator(in Photon currentPhoton, int index, float maximumRadius) {

	int l = index - BOXED_PHOTONS;
	int r = index - BOXED_PHOTONS;

	int currentMorton = Morton[Permutation[index]];

	for (int i = 0; i < 10; i++) {
		int currentMask = ~((1 << (i * 3)) - 1);
		int currentBlock = currentMorton & currentMask;
		float mortonBlockRadius = (1 << i) / 1024.0;

		int inc = BOXED_PHOTONS;

		// expand l and r considering all photons inside current block (currentMask)
		while (l >= 0 && ((Morton[Permutation[l]] & currentMask) == currentBlock))
		{
			l -= inc;
			inc <<= 1;
		}

		inc = BOXED_PHOTONS;

		while (r < bufferSize && ((Morton[Permutation[r]] & currentMask) == currentBlock))
		{
			r += inc;
			inc <<= 1;
		}
		if ((r - l) >= DESIRED_PHOTONS * 4 / 3.14159)
		{
			return sqrt(mortonBlockRadius * mortonBlockRadius * DESIRED_PHOTONS / (r - l) * 4 / 3.14);
		}
		if (mortonBlockRadius >= maximumRadius)
			return maximumRadius;

		//return mortonBlockRadius * 1.73 / pow((r - l)/ (float)DESIRED_PHOTONS, 0.5);
	}
	return maximumRadius;
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	AABB box = (AABB)0;
	box.minimum = 10000;
	box.maximum = -10000;

	for (int i = 0; i < BOXED_PHOTONS; i++)
	{
		int photonIdx = index * BOXED_PHOTONS + i;

		Photon currentPhoton = Photons[Permutation[photonIdx]];
		float radius = 0;

		if (any(currentPhoton.Intensity))
		{
			radius = clamp(MortonEstimator(currentPhoton, photonIdx, clamp(RadiiFactor[Permutation[photonIdx]], 1, 8) * PHOTON_RADIUS), PHOTON_RADIUS * 0.01, PHOTON_RADIUS * 8);
			//radius = clamp(MortonEstimator(currentPhoton, photonIdx, PHOTON_RADIUS), PHOTON_RADIUS * 0.01, PHOTON_RADIUS);

			box.minimum = min(box.minimum, currentPhoton.Position - radius);
			box.maximum = max(box.maximum, currentPhoton.Position + radius);
		}

		radii[photonIdx] = max(0.0001,radius);
		AllocatedPhotons[photonIdx] = currentPhoton;
	}

	if (box.maximum.x <= box.minimum.x)
	{
		float3 pos = 0;// 2 * float3(x, y, z) - 1;
		//float3 pos = Photons[Permutation[0]].Position;
		box.minimum = pos - 0.0001;// -radius * 0.0001;
		box.maximum = pos + 0.0001;// +radius * 0.0001;
	}
	PhotonAABBs[index] = box;
}