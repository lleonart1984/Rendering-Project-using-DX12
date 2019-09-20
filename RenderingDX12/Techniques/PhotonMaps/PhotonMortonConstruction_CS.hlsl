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

RWStructuredBuffer<AABB> PhotonAABBs	: register (u0);
RWStructuredBuffer<float> radii			: register (u1);

float MortonEstimator(in Photon currentPhoton, int index) {
	int l = index - 1;
	int r = index + 1;

	for (int i = 0; i < 10; i++) {
		int currentMask = ~((1 << (i * 3)) - 1);
		int currentBlock = Morton[index] & currentMask;
		float mortonBlockRadius = (1 << i) / 1024.0;

		// expand l and r considering all photons inside current block (currentMask)
		while (l >= 0 && ((Morton[l] & currentMask) == currentBlock))
			l--;
		while (r < PHOTON_DIMENSION * PHOTON_DIMENSION - 1 && ((Morton[r] & currentMask) == currentBlock))
			r++;
		if ((r - l) >= DESIRED_PHOTONS * 4 / 3.14159)
		{
			return pow(mortonBlockRadius * mortonBlockRadius * DESIRED_PHOTONS / (r - l) * 4 / 3.14, 1.0 / 2);
		}
		if (mortonBlockRadius >= PHOTON_RADIUS)
			return PHOTON_RADIUS;

		//return mortonBlockRadius * 1.73 / pow((r - l)/ (float)DESIRED_PHOTONS, 0.5);
	}
	return PHOTON_RADIUS;
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	float radius = PHOTON_RADIUS;
	AABB box = (AABB)0;

	Photon currentPhoton = Photons[index];

	if (any(currentPhoton.Intensity))
	{
		radius = clamp(MortonEstimator(currentPhoton, index), PHOTON_RADIUS * 0.001, PHOTON_RADIUS);

		box.minimum = currentPhoton.Position - radius;
		box.maximum = currentPhoton.Position + radius;
	}
	else {
		radius = 0;

		//float3 pos = 0;// 2 * float3(x, y, z) - 1;
		float3 pos = Photons[0].Position;

		//box.minimum = pos - 0.00001;// -radius * 0.0001;
		//box.maximum = pos + 0.00001;// +radius * 0.0001;
	}
	radii[index] = radius;
	PhotonAABBs[index] = box;
}