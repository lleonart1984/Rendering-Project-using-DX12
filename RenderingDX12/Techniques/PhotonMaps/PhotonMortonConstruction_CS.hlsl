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

StructuredBuffer<Photon> Photons		: register (t0);
StructuredBuffer<int> Morton			: register (t1);
StructuredBuffer<int> Permutation		: register (t2);

RWStructuredBuffer<AABB> PhotonAABBs	: register (u0);
RWStructuredBuffer<float> radii			: register (u1);

static const int bufferSize = PHOTON_DIMENSION * PHOTON_DIMENSION - 1;

float MortonEstimator(in Photon currentPhoton, int index) {

	int l = index - 1;
	int r = index + 1;

	int currentMorton = Morton[Permutation[index]];

	for (int i = 0; i < 10; i++) {
		int currentMask = ~((1 << (i * 3)) - 1);
		int currentBlock = currentMorton & currentMask;
		float mortonBlockRadius = (1 << i) / 1024.0;

		int inc = 1;

		// expand l and r considering all photons inside current block (currentMask)
		while (l >= 0 && ((Morton[Permutation[l]] & currentMask) == currentBlock))
		{
			l -= inc;
			inc <<= 1;
		}
		
		inc = 1;

		while (r < bufferSize && ((Morton[Permutation[r]] & currentMask) == currentBlock))
		{
			r += inc;
			inc <<= 1;
		}
		if ((r - l) >= DESIRED_PHOTONS * 4 / 3.14159)
		{
			return sqrt(mortonBlockRadius * mortonBlockRadius * DESIRED_PHOTONS / (r - l) * 4 / 3.14);
		}
		if (mortonBlockRadius >= PHOTON_RADIUS)
			return PHOTON_RADIUS;

		//return mortonBlockRadius * 1.73 / pow((r - l)/ (float)DESIRED_PHOTONS, 0.5);
	}
	return PHOTON_RADIUS;
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	float radius = PHOTON_RADIUS;
	AABB box = (AABB)0;

	Photon currentPhoton = Photons[Permutation[index]];

	if (any(currentPhoton.Intensity))
	{
		radius = clamp(MortonEstimator(currentPhoton, index), PHOTON_RADIUS * 0.001, PHOTON_RADIUS);

		box.minimum = currentPhoton.Position - radius;
		box.maximum = currentPhoton.Position + radius;
	}
	else {
		radius = 0;

		float3 pos = 0;// 2 * float3(x, y, z) - 1;
		//float3 pos = Photons[Permutation[0]].Position;

		box.minimum = pos - 0.000001;// -radius * 0.0001;
		box.maximum = pos + 0.000001;// +radius * 0.0001;
	}
	radii[Permutation[index]] = radius;
	PhotonAABBs[index] = box;
}