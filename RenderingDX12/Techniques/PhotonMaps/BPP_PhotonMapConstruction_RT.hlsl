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

			if (any(Photons[adj].Intensity) && d < 2*PHOTON_RADIUS) // only consider valid photons
			{
				count++;
				float kernel = 1;// / (1 + 10 * d / PHOTON_RADIUS);// 4 * pow(1 - saturate(d / (2 * PHOTON_RADIUS)), 0.25) + 0.1;
				kInt += kernel;
				total += kernel * d;
			}
		}
	if (count < DESIRED_PHOTONS / 10)
		return PHOTON_RADIUS;
	return total * 0.5 / kInt;
	//((maxC - minC + 1)*(maxR - minR + 1));
}

#define T 5

float HistogramEstimationBox(int index) {
	int row = index / PHOTON_DIMENSION;
	int col = index % PHOTON_DIMENSION;
	// will be considered approx (2*radius+1)^2 / 2 nearest photons
	int radius = 7; // 3 -> ~25, 4 -> ~40
	int minR = max(0, row - radius);
	int maxR = min(PHOTON_DIMENSION - 1, row + radius);
	int minC = max(0, col - radius);
	int maxC = min(PHOTON_DIMENSION - 1, col + radius);
	float3 currentPosition = Photons[index].Position;
	int histogram[T];
	for (int i = 0; i < T; i++)
		histogram[i] = 0;

	for (int r = minR; r <= maxR; r++)
		for (int c = minC; c <= maxC; c++)
		{
			int adj = r * PHOTON_DIMENSION + c;

			float3 adjPhotonPos = Photons[adj].Position;
			float d = distance(adjPhotonPos, currentPosition);

			if (any(Photons[adj].Intensity) && d < PHOTON_RADIUS) // only consider valid photons
			{
				int idx = min(T - 1, T * d / PHOTON_RADIUS);
				histogram[idx]++;
			}
		}
	int counting = DESIRED_PHOTONS;
	for (int i = 0; i < T; i++)
	{
		if (counting < histogram[i])
			return PHOTON_RADIUS * (i / (float)T + (1.0 / T) * counting / (float)histogram[i]);
		counting -= histogram[i];
	}
	return PHOTON_RADIUS;
}


[shader("raygeneration")]
void Main()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int index = raysIndex.y * raysDimensions.x + raysIndex.x;

	float radius = PHOTON_RADIUS;

	if (any(Photons[index].Intensity)) {
		switch (ADAPTIVE_STRATEGY) {
		case 0: // No adaptive at all
			radius = NoAdaptiveBox(index);
			break;
		case 1: // Media
			radius = HistogramEstimationBox(index);
			break;
		}

		// clamp shrinking to the quarter, and the enlarging to the fourth times
		radius = clamp(radius, 0.0001, PHOTON_RADIUS);
		radii[index] = radius;

		AABB box = (AABB)0;
		box.minimum = Photons[index].Position - radius;
		box.maximum = Photons[index].Position + radius;
		PhotonAABBs[index] = box;
	}
	else {
		radii[index] = 0;// radius;

		float x = raysIndex.x / (float)PHOTON_DIMENSION;
		float y = raysIndex.y / (float)PHOTON_DIMENSION;
		float z = 2*(abs(index * 0x888888) % 128) / 128.0 - 1;

		float3 pos = 2 * float3(x, y, z) - 1;

		AABB box = (AABB)0;
		box.minimum = pos - 0.00001;// -radius * 0.0001;
		box.maximum = pos + 0.00001;// +radius * 0.0001;
		PhotonAABBs[index] = box;
	}
}