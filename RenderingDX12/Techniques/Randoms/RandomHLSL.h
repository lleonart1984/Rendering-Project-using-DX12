#ifndef RANDOMHLSL_H
#define RANDOMHLSL_H

// HLSL code for random functions

#include "../CommonGI/Definitions.h"

uint rand_xorshift(inout uint rng_state)
{
	// Xorshift algorithm from George Marsaglia's paper
	rng_state ^= (rng_state << 13);
	rng_state ^= (rng_state >> 17);
	rng_state ^= (rng_state << 5);
	return rng_state;
}

float random(inout uint rng_state)
{
	return rand_xorshift(rng_state) * (1.0 / 4294967296.0);
}

float3 randomDirection(inout uint rng_state)
{
	float r1 = random(rng_state);
	float r2 = random(rng_state) * 2 - 1;
	float sqrR2 = r2 * r2;
	float two_pi_by_r1 = two_pi * r1;
	float sqrt_of_one_minus_sqrR2 = sqrt(1.0 - sqrR2);
	float x = cos(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float y = sin(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float z = r2;
	return float3(x, y, z);
}

float3 randomHSDirection(inout uint rng_state, float3 N, out float NdotD)
{
	float r1 = random(rng_state);
	float r2 = random(rng_state) * 2 - 1;
	float sqrR2 = r2 * r2;
	float two_pi_by_r1 = two_pi * r1;
	float sqrt_of_one_minus_sqrR2 = sqrt(1.0 - sqrR2);
	float x = cos(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float y = sin(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float z = r2;
	float3 d = float3(x, y, z);
	NdotD = dot(N, d);
	d *= (NdotD < 0 ? -1 : 1);
	NdotD *= NdotD < 0 ? -1 : 1;
	return d;
}

void initializeRandom(inout uint rng_state) {
	int seed = rng_state;
	[loop]
	for (int i = 0; i < seed % 10 + 3; i++)
		random(rng_state);
}

uint StartRandomSeedForRay(uint2 gridDimensions, int maxBounces, uint2 raysIndex, int bounce, int frame) {
	uint index = 0;
	int dim = 1;
	index += raysIndex.x * dim;
	dim *= gridDimensions.x;
	index += raysIndex.y * dim;
	dim *= gridDimensions.y;
	index += bounce * dim;
	dim *= maxBounces;
	index += frame * dim;
	initializeRandom(index);
	return index;
}

#endif // RANDOMHLSL_H