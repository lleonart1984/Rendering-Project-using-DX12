#ifndef RANDOMHLSL_H
#define RANDOMHLSL_H

// HLSL code for random functions

#include "../CommonGI/Definitions.h"

static uint rng_state;

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

float3 randomDirection()
{
	float r1 = random();
	float r2 = random() * 2 - 1;
	float sqrR2 = r2 * r2;
	float two_pi_by_r1 = two_pi * r1;
	float sqrt_of_one_minus_sqrR2 = sqrt(1.0 - sqrR2);
	float x = cos(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float y = sin(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float z = r2;
	return float3(x, y, z);
}

float3 randomHSDirection(float3 N, out float NdotD)
{
	float r1 = random();
	float r2 = random() * 2 - 1;
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

void initializeRandom(uint seed) {
	rng_state = seed;// ^ 0xFEFE;
	[loop]
	for (int i = 0; i < seed % 10 + 3; i++)
		random();
}

void StartRandomSeedForRay(uint2 gridDimensions, int maxBounces, uint2 raysIndex, int bounce, int frame) {
	int index = 0;
	int dim = 1;
	index += raysIndex.x * dim;
	dim *= gridDimensions.x;
	index += raysIndex.y * dim;
	dim *= gridDimensions.y;
	index += bounce * dim;
	dim *= maxBounces;
	index += frame * dim;
	initializeRandom(index);
}

#endif // RANDOMHLSL_H