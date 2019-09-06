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
StructuredBuffer<int> Permutation		: register (t1);

RWStructuredBuffer<AABB> PhotonAABBs	: register (u0);
RWStructuredBuffer<float> radii			: register (u1);

int split3(int value) {

	int ans = value & 0x3FF; // allow 10 bits only.

	ans = (ans | ans << 16) & 0xff0000ff;
	// poniendo 8 ceros entre cada grupo de 4 bits.
	// shift left 8 bits y despues hacer OR entre ans y 0001000000001111000000001111000000001111000000001111000000000000
	ans = (ans | ans << 8) & 0x0f00f00f;
	// poniendo 4 ceros entre cada grupo de 2 bits.
	// shift left 4 bits y despues hacer OR entre ans y 0001000011000011000011000011000011000011000011000011000011000011
	ans = (ans | ans << 4) & 0xc30c30c3;
	// poniendo 2 ceros entre cada bit.
	// shift left 2 bits y despues hacer OR entre ans y 0001001001001001001001001001001001001001001001001001001001001001
	ans = (ans | ans << 2) & 0x49249249;
	return ans;
}

int morton(int3 pos) {
	//return pos.x;
	return split3(pos.x) | (split3(pos.y) << 1) | (split3(pos.z) << 2);
}

int mortonf(float3 pos) {
	return morton(int3(saturate((pos * 0.5) + 0.5)*((1 << 9) - 1)));
}

[shader("raygeneration")]
void Main()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int index = raysIndex.y * raysDimensions.x + raysIndex.x;
	//int rightIndex = min(PHOTON_DIMENSION*PHOTON_DIMENSION - 1, index + DESIRED_PHOTONS);
	int leftIndex = max(0, index - 1);

	float radius = PHOTON_RADIUS;

	Photon currentPhoton = Photons[Permutation[index]];

	if (any(currentPhoton.Intensity)){
		Photon leftPhoton = Photons[Permutation[leftIndex]];
		//Photon rightPhoton = Photons[Permutation[rightIndex]];

		int currentMorton = mortonf(currentPhoton.Position);
		int leftMorton = mortonf(leftPhoton.Position);

		if (leftMorton <= currentMorton)
			radius = 0.01* PHOTON_RADIUS;// .1*PHOTON_RADIUS;
		else
			radius = 1 * PHOTON_RADIUS;

		//radius = distance(currentPhoton.Position, leftPhoton.Position)*0.5;
		
		// clamp shrinking to the quarter, and the enlarging to the fourth times
		radius = clamp(radius, 0.0001, PHOTON_RADIUS);
		radii[Permutation[index]] = radius;

		AABB box = (AABB)0;
		box.minimum = currentPhoton.Position - radius;
		box.maximum = currentPhoton.Position + radius;
		PhotonAABBs[Permutation[index]] = box;
	}
	else {
		radii[Permutation[index]] = 0;// radius;

		float x = raysIndex.x / (float)PHOTON_DIMENSION;
		float y = raysIndex.y / (float)PHOTON_DIMENSION;
		float z = 2 * (abs(index * 0x888888) % 128) / 128.0 - 1;

		float3 pos = 2 * float3(x, y, z) - 1;

		AABB box = (AABB)0;
		box.minimum = pos - 0.00001;// -radius * 0.0001;
		box.maximum = pos + 0.00001;// +radius * 0.0001;
		PhotonAABBs[Permutation[index]] = box;
	}
}