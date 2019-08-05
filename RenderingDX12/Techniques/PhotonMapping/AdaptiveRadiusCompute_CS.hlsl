/// Compute shader to estimate splatting radius for each photon
/// Using the closest photons in emission image as a reference

// Requires: Photon buffer (emission image)
// Ensures: Buffer with the estimated radius for each photon

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

StructuredBuffer<Photon> Photons	: register (t0);

RWStructuredBuffer<float> radii		: register (u0);

float NoAdaptiveBox(int index) {
	return PHOTON_RADIUS;
}

float MediaAsMedianEstimationBox(int index) {
	int row = index / PHOTON_DIMENSION;
	int col = index % PHOTON_DIMENSION;
	// will be considered approx (2*radius+1)^2 / 2 nearest photons
	int radius = 3; // 3 -> ~25, 4 -> ~40

	int minR = max(0, row - radius);
	int maxR = min(PHOTON_DIMENSION - 1, row + radius);
	int minC = max(0, col - radius);
	int maxC = min(PHOTON_DIMENSION - 1, col + radius);
	float3 currentPosition = Photons[index].Position;
	float total = 0;
	int count = 0;
	for (int r = minR; r <= maxR; r++)
		for (int c = minC; c <= maxC; c++)
		{
			int adj = r * PHOTON_DIMENSION + c;

			float3 adjPhotonPos = Photons[adj].Position;

			if (any(Photons[adj].Intensity)) // only consider valid photons
			{
				count++;
				total += distance(adjPhotonPos, currentPosition);
			}
		}
	return total / count;
		//((maxC - minC + 1)*(maxR - minR + 1));
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	float radius = 0;

	switch (ADAPTIVE_STRATEGY) {
	case 0: // No adaptive at all
		radius = NoAdaptiveBox(index);
		break;
	case 1: // Media
		radius = MediaAsMedianEstimationBox(index);
		break;
	}

	// clamp shrinking to the quarter, and the enlarging to the fourth times
	radius = clamp(radius, PHOTON_RADIUS * 0.25, PHOTON_RADIUS * 4);

	radii[index] = radius;
}