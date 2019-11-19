#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

// 0 - No adaptive strategy
// 1 - Use radii factor to scale fixed radius
// 2 - Use media estimator
// 3 - Use multiplication of radii factor times media estimator

#ifndef ADAPTIVE_STRATEGY
#define ADAPTIVE_STRATEGY 0
#endif

struct AABB {
	float3 minimum;
	float3 maximum;
};

StructuredBuffer<Photon> Photons		: register (t0);
StructuredBuffer<float> RadiusFactors	: register (t1);

RWStructuredBuffer<AABB> PhotonAABBs	: register (u0);
RWStructuredBuffer<float> radii			: register (u1);

float NoAdaptiveBox(int index) {
	return PHOTON_RADIUS;
}

float MediaAsMedianEstimationBox(int index) {
	int row = index / PHOTON_DIMENSION;
	int col = index % PHOTON_DIMENSION;
	// will be considered approx (2*radius+1)^2 / 2 nearest photons
	int radius = 50; // 3 -> ~25, 4 -> ~40
	float power = ADAPTIVE_POWER;
	int minR = max(0, row - radius);
	int maxR = min(PHOTON_DIMENSION - 1, row + radius);
	int minC = max(0, col - radius);
	int maxC = min(PHOTON_DIMENSION - 1, col + radius);
	float3 currentPosition = Photons[index].Position;
	float total = 0;
	float kInt = 0;
	int count = 0;
	for (int r = minR; r <= maxR; r++)
		for (int c = minC; c <= maxC; c++)
		{
			int adj = r * PHOTON_DIMENSION + c;

			float3 adjPhotonPos = Photons[adj].Position;
			float d = distance(adjPhotonPos, currentPosition);

			if (any(Photons[adj].Intensity) && d < 8 * PHOTON_RADIUS) // only consider valid photons
			{
				count++;
				float kernel = 1;// / (1 + 10 * d / PHOTON_RADIUS);// 4 * pow(1 - saturate(d / (2 * PHOTON_RADIUS)), 0.25) + 0.1;
				kInt += kernel;
				total += kernel * d;
			}
		}
	if (count < DESIRED_PHOTONS / 20)
		return PHOTON_RADIUS;
	return total * 0.25 / kInt;
	//((maxC - minC + 1)*(maxR - minR + 1));
}

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	int index = DTid.x;

	float radius;
	float3 boxCenter = 0;

	if (any(Photons[index].Intensity)) {
		if (ADAPTIVE_STRATEGY / 2)
			radius = MediaAsMedianEstimationBox(index);
		else
			radius = NoAdaptiveBox(index);

		if (ADAPTIVE_STRATEGY % 2) // use radii factor
			radius *= RadiusFactors[index];

		// clamp shrinking to the quarter, and the enlarging to the fourth times
		radius = clamp(radius, 0.0001, 8*PHOTON_RADIUS);
		boxCenter = Photons[index].Position;
	}
	else {
		radius = 0;
		boxCenter = 0;
	}

	AABB box = (AABB)0;
	box.minimum = boxCenter - radius;
	box.maximum = boxCenter + radius;
	PhotonAABBs[index] = box;
	radii[index] = radius;
}