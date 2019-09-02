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

RWStructuredBuffer<float> radii			: register (u0);
RWStructuredBuffer<AABB> PhotonAABBs	: register (u1);

float NoAdaptiveBox(int index) {
	return PHOTON_RADIUS;
}

void AccPhotonAt(float3 currentPosition, int r, int c, inout int count, inout float total, inout float kernelInt) {
	int adj = r * PHOTON_DIMENSION + c;

	float3 adjPhotonPos = Photons[adj].Position;
	float d = distance(adjPhotonPos, currentPosition);

	if (any(Photons[adj].Intensity))// && d < PHOTON_RADIUS) // only consider valid photons
	{
		count += 1;// d < PHOTON_RADIUS * 0.2;
		float kernel = 1;// 2 * (1 - saturate(pow(d / PHOTON_RADIUS, 1))) + 0.01;
		total += kernel * d;// *pow(d, 0.5);
		kernelInt += kernel;
	}
}

float MediaAsMedianEstimationBox(int index) {
	int row = index / PHOTON_DIMENSION;
	int col = index % PHOTON_DIMENSION;
	// will be considered approx (2*radius+1)^2 / 2 nearest photons
	int radius = 5; // 3 -> ~25, 4 -> ~40
	float power = ADAPTIVE_POWER;
	//int minR = max(0, row - radius);
	//int maxR = min(PHOTON_DIMENSION - 1, row + radius);
	//int minC = max(0, col - radius);
	//int maxC = min(PHOTON_DIMENSION - 1, col + radius);
	//int countOfPhotons = (maxC - minC)*(maxR - minR);
	float3 currentPosition = Photons[index].Position;
	float total = 0;
	float kernelInt = 0;
	int count = 0;

	int R = 0;
	while (R <= radius)
	{
		int minR = max(0, row - R);
		int maxR = min(PHOTON_DIMENSION - 1, row + R);
		int minC = max(0, col - R);
		int maxC = min(PHOTON_DIMENSION - 1, col + R);

		if (row - R >= 0) // Draw top curve
		{
			int r = row - R;
			for (int c = minC; c <= maxC; c++)
				AccPhotonAt(currentPosition, r, c, count, total, kernelInt);
		}

		if (row + R < PHOTON_DIMENSION) // Draw top curve
		{
			int r = row + R;
			for (int c = minC; c <= maxC; c++)
				AccPhotonAt(currentPosition, r, c, count, total, kernelInt);
		}

		if (col - R >= 0) {
			int c = col - R;
			for (int r = minR + 1; r < maxR; r++)
				AccPhotonAt(currentPosition, r, c, count, total, kernelInt);
		}

		if (col + R < PHOTON_DIMENSION) {
			int c = col + R;
			for (int r = minR + 1; r < maxR; r++)
				AccPhotonAt(currentPosition, r, c, count, total, kernelInt);
		}

		R++;
	}

	//for (int r = minR; r <= maxR; r++)
	//	for (int c = minC; c <= maxC; c++)
	//	{
	//		int adj = r * PHOTON_DIMENSION + c;

	//		float3 adjPhotonPos = Photons[adj].Position;
	//		float d = distance(adjPhotonPos, currentPosition);

	//		if (any(Photons[adj].Intensity)) // only consider valid photons
	//		{
	//			count++;
	//			float kernel = 1.0;// / (1 + abs(r - row) + abs(c - col));// 2 * (1 - saturate(d / PHOTON_RADIUS)) + 1;
	//			total += kernel * pow(d, 1);
	//			kernelInt += kernel;
	//		}
	//	}
	if (count < 4)
		return PHOTON_RADIUS;
	return total / kernelInt;
	//((maxC - minC + 1)*(maxR - minR + 1));
}


[shader("raygeneration")]
void RTMainProgram()
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
			radius = MediaAsMedianEstimationBox(index);
			break;
		}

		// clamp shrinking to the quarter, and the enlarging to the fourth times
		radius = clamp(radius, 0.001, PHOTON_RADIUS);
		radii[index] = radius;

		AABB box = (AABB)0;
		box.minimum = Photons[index].Position - radius;
		box.maximum = Photons[index].Position + radius;
		PhotonAABBs[index] = box;
	}
	else {
		radii[index] = 0;// radius;

		AABB box = (AABB)0;
		PhotonAABBs[index] = box;
	}
}