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
RWStructuredBuffer<Photon> SortedPhotons		: register (u2);

static const int bufferSize = PHOTON_DIMENSION * PHOTON_DIMENSION - 1;

float MortonEstimator2(in Photon currentPhoton, int index, float maximumRadius)
{
	float total = 0;
	float kernelInt = 0;
	int step = DESIRED_PHOTONS / 10 + 1;
	for (int i = 1; i < DESIRED_PHOTONS && i - index >= 0; i += step) {
		float d = length(Photons[Permutation[index - i]].Position - currentPhoton.Position);
		if (d < maximumRadius)
		{
			float kernel = 1;// 2 * (1 - i / (float)DESIRED_PHOTONS);
			total += d * kernel;
			kernelInt += kernel;
		}
	}
	
	for (int i = 1; i < DESIRED_PHOTONS && i + index <= bufferSize; i += step) {
		float d = length(Photons[Permutation[index + i]].Position - currentPhoton.Position);
		if (d < maximumRadius) {
			float kernel = 1;// 2 * (1 - i / (float)DESIRED_PHOTONS);
			total += d * kernel;
			kernelInt += kernel;
		}
	}

	float estimatedRadius = sqrt(total / kernelInt / 3.14159);

	return estimatedRadius * 0.25;
}

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
		do
		{
			int ratio = 1 + pow(Permutation[l],5) % 4;
			l -= max(1, inc * ratio / 4);
			inc <<= (Permutation[l] % 2);
		} while (l >= 0 && ((Morton[Permutation[l]] & currentMask) == currentBlock));

		inc = BOXED_PHOTONS;

		do
		{
			int ratio = 1 + pow(Permutation[r],3) % 4;
			r += max(1, inc * ratio / 4);
			inc <<= (Permutation[r] % 2);
		} while (r < bufferSize && ((Morton[Permutation[r]] & currentMask) == currentBlock));

		if (mortonBlockRadius >= maximumRadius)
			return maximumRadius;

		if ((r - l) >= DESIRED_PHOTONS)
		{
			//return sqrt(mortonBlockRadius * mortonBlockRadius * DESIRED_PHOTONS / (r - l) * 4 / 3.14);
			//return pow(mortonBlockRadius * mortonBlockRadius * mortonBlockRadius * DESIRED_PHOTONS / (r - l), 0.33333);
			//return mortonBlockRadius * DESIRED_PHOTONS / (r - l) ;
			return mortonBlockRadius;// *sqrt(max(0.125, DESIRED_PHOTONS / (r - l) * 4 / 3.14159));
		}


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
#if ADAPTIVE_STRATEGY == 1
			radius = clamp(MortonEstimator(currentPhoton, photonIdx, clamp(RadiiFactor[Permutation[photonIdx]], 1, 8) * PHOTON_RADIUS), 0.0001, PHOTON_RADIUS * 8);
			//radius = clamp(MortonEstimator(currentPhoton, photonIdx, PHOTON_RADIUS), PHOTON_RADIUS * 0.001, PHOTON_RADIUS);
#else
			radius = PHOTON_RADIUS;
#endif

			box.minimum = min(box.minimum, currentPhoton.Position - radius);
			box.maximum = max(box.maximum, currentPhoton.Position + radius);
		}

		radii[photonIdx] = max(0.0001,radius);
		SortedPhotons[photonIdx] = currentPhoton;
	}

	if (box.maximum.x <= box.minimum.x)
	{
		float3 pos = 0;// 2 * float3(x, y, z) - 1;
		//float3 pos = Photons[Permutation[0]].Position;
		//box.minimum = pos - 0.0001;// -radius * 0.0001;
		//box.maximum = pos + 0.0001;// +radius * 0.0001;
		box.minimum = 0;
		box.maximum = 0;
	}
	PhotonAABBs[index] = box;
}