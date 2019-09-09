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


float HistogramEstimator(in Photon currentPhoton, int index) {

	rng_state = index;

	for (int i = 0; i < index % 5 + 3; i++)
		random();

	float histogram[T];
	for (int i = 0; i < T; i++)
		histogram[i] = 0;

	int samples = 1000;
	int Mutation = 1000;
	int X = index;
	float dX = 0;
	float pdfX = 1;

	for (int i = 0; i < samples; i++) {
		int newX = clamp(X + (random() * 2 - 1)*(Mutation + abs(X - index)), 0, PHOTON_DIMENSION*PHOTON_DIMENSION - 1);
		Photon p = Photons[newX];
		float newXD = distance(currentPhoton.Position, p.Position);
		float newXPdf = !any(p.Intensity) ? 0.0 : 1.0 / exp(0.05*newXD/ PHOTON_RADIUS);
		bool accept = random() < (newXPdf >= pdfX ? 1 : newXPdf / pdfX);

		if (accept)
		{
			X = newX;
			dX = newXD;
			pdfX = newXPdf;
		}
		int idx = max(0, min(T - 1, (int)(T * (dX / PHOTON_RADIUS))));
		histogram[idx] += (dX < PHOTON_RADIUS) / pdfX / exp(0.05);
	}

	float counting = DESIRED_PHOTONS;
	for (int i = 0; i < T; i++)
	{
		if (counting <= histogram[i])
			return PHOTON_RADIUS * (i + 1.0) / T;
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
	AABB box = (AABB)0;

	Photon currentPhoton = Photons[index];

	if (any(currentPhoton.Intensity))
	{
		radius = clamp(HistogramEstimator(currentPhoton, index), PHOTON_RADIUS*0.001, PHOTON_RADIUS);

		box.minimum = currentPhoton.Position - radius;
		box.maximum = currentPhoton.Position + radius;
	}
	else {
		radius = 0;

		float x = raysIndex.x / (float)PHOTON_DIMENSION;
		float y = raysIndex.y / (float)PHOTON_DIMENSION;
		float z = 2 * (abs(index * 0x888888) % 128) / 128.0 - 1;

		float3 pos = Photons[0].Position;// 2 * float3(x, y, z) - 1;

		box.minimum = pos - 0.00001;// -radius * 0.0001;
		box.maximum = pos + 0.00001;// +radius * 0.0001;
	}
	radii[index] = radius;
	PhotonAABBs[index] = box;
}