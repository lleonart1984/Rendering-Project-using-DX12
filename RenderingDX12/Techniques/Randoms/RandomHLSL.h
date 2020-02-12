#ifndef RANDOMHLSL_H
#define RANDOMHLSL_H

// HLSL code for random functions

#include "../CommonGI/Definitions.h"

struct ULONG
{
	uint h, l;
};

ULONG XOR (ULONG x, ULONG y) {
	ULONG r = { 0, 0 };
	r.h = x.h ^ y.h;
	r.l = x.l ^ y.l;
	return r;
}

ULONG RSHIFT(ULONG x, int p) {
	ULONG r = { 0, 0 };
	r.l = (x.l >> p) | (x.h << (32 - p));
	r.h = x.h >> p;
	return r;
}

ULONG LSHIFT(ULONG x, int p) {
	ULONG r = { 0, 0 };
	r.h = (x.h << p) | (x.l >> (32 - p));
	r.l = x.l << p;
	return r;
}

ULONG ADD(ULONG x, ULONG y) {
	ULONG r = { 0, 0 };
	r.l = x.l + y.l;
	r.h = x.h + y.h + (r.l < x.l); // carry a 1 when ls addition overflow
	return r;
}

ULONG MUL(uint x, uint y) {
	int h1 = x >> 16;
	int l1 = x & 0xFFFF;
	int h2 = y >> 16;
	int l2 = y & 0xFFFF;

	ULONG r1;
	r1.h = h1 * h2;
	r1.l = l1 * l2;

	ULONG r2 = { l1 * h2, 0 };
	ULONG r3 = { l2 * h1, 0 };

	return ADD(r1, ADD(r2, r3));
}

ULONG MUL(ULONG x, ULONG y) {
	return ADD(
		LSHIFT(MUL(x.h, y.l), 32),
		ADD(
			LSHIFT(MUL(x.l, y.h), 32),
			MUL(x.l, y.l)
		));
}

//#include "MarsagliaRandoms.h"
//#include "MarsagliasXorWow.h"
#include "XorShiftAster.h"

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

void StartRandomSeedForRay(uint2 gridDimensions, int maxBounces, uint2 raysIndex, int bounce, int frame) {
	uint index = 0;
	uint dim = 1;
	index += raysIndex.x * dim;
	dim *= gridDimensions.x;
	index += raysIndex.y * dim;
	dim *= gridDimensions.y;
	index += bounce * dim;
	dim *= maxBounces;

	ULONG INDEX = { 0, index };
	ULONG FRAME = { index%13, frame };
	ULONG DIM = { 0, dim };

	//index += frame * dim;
	INDEX = ADD(INDEX, MUL(FRAME, DIM));

	initializeRandom(INDEX);

	for (int i = 0; i < 23 + index % 13; i++)
		random();
}

void StartRandomSeedForThread(uint gridDimensions, int maxBounces, uint index, int bounce, int frame) {
	uint dim = 1;
	dim *= gridDimensions;
	index += bounce * dim;
	dim *= maxBounces;

	ULONG INDEX = { 0, index };
	ULONG FRAME = { index % 13, frame };
	ULONG DIM = { 0, dim };

	//index += frame * dim;
	INDEX = ADD(INDEX, MUL(FRAME, DIM));

	initializeRandom(INDEX);

	for (int i = 0; i < 23 + index % 13; i++)
		random();
}

#endif // RANDOMHLSL_H