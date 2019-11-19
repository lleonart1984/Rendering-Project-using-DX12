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

			if (any(Photons[adj].Intensity) && d < 8*PHOTON_RADIUS) // only consider valid photons
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

#define T 20

shared static uint rng_state;

uint rand_xorshift()
{
	// Xorshift algorithm from George Marsaglia's paper
	rng_state ^= (rng_state << 13);
	rng_state ^= (rng_state >> 17);
	rng_state ^= (rng_state << 5);
	return rng_state;
}

float random()
{
	return rand_xorshift() * (1.0 / 4294967296.0);
}

float pdf(float d) {
	float x = min(PHOTON_RADIUS, d)/PHOTON_RADIUS;
	return 1;// / (1 + x * x) + 0.1;
}

float HistogramEstimationBox(int index) {
	int row = index / PHOTON_DIMENSION;
	int col = index % PHOTON_DIMENSION;

	float3 currentPosition = Photons[index].Position;
	float2 imagePos = float2(col / (float)PHOTON_DIMENSION, row / (float)PHOTON_DIMENSION);

	rng_state = index;
	int samples = 10000; 

	float histogram[T];
	for (int i = 0; i < T; i++)
		histogram[i] = 0;

	int2 X = int2(col, row); // start metropolis in current photon
	float dX = 0;
	float pdfX = pdf(dX); // initial distance is 0.
	bool isLit = true;
	int R = 10;
	for (int i = 0; i < samples; i++)
	{
		int2 nX = int2(random()*PHOTON_DIMENSION, random()*PHOTON_DIMENSION);// clamp(X + int2(random() * R - R / 2, random() * R - R / 2), 0, PHOTON_DIMENSION - 1);
		int adj = nX.y * PHOTON_DIMENSION + nX.x;
		float3 adjPhotonPos = Photons[adj].Position;
		float d = distance(adjPhotonPos, currentPosition);
		bool adjIsLit = any(Photons[adj].Intensity);
		float npdfX = pdf(d);

		float accept = npdfX >= pdfX ? 1 : npdfX / pdfX;
		if (random() < accept)
		{
			X = nX;
			pdfX = npdfX;
			dX = d;
			isLit = adjIsLit;
		}

		if (isLit && dX < PHOTON_RADIUS) // only consider valid photons
		{
			int idx = min(T - 1, T * dX / PHOTON_RADIUS);
			histogram[idx] += PHOTON_DIMENSION * PHOTON_DIMENSION / pdfX / samples;
		}
	}

	float counting = DESIRED_PHOTONS;
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
			radius = MediaAsMedianEstimationBox(index);
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

		/*float x = raysIndex.x / (float)PHOTON_DIMENSION;
		float y = raysIndex.y / (float)PHOTON_DIMENSION;
		float z = 2*(abs(index * 0x888888) % 128) / 128.0 - 1;

		float3 pos = 2 * float3(x, y, z) - 1;*/
		float3 pos = 0;

		AABB box = (AABB)0;
		box.minimum = pos - 0.00001;// -radius * 0.0001;
		box.maximum = pos + 0.00001;// +radius * 0.0001;
		PhotonAABBs[index] = box;
	}
}