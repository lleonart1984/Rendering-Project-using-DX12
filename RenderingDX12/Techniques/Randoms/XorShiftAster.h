
shared static ULONG rng_state;

/* The state array must be initialized to not be all zero in the first four words */
uint xorshift64s()
{
	//x ^= x >> 12; // a
	rng_state = XOR(rng_state, RSHIFT(rng_state, 12));
	//x ^= x << 25; // b
	rng_state = XOR(rng_state, LSHIFT(rng_state, 25));
	//x ^= x >> 27; // c
	rng_state = XOR(rng_state, RSHIFT(rng_state, 27));

	ULONG c = { 0x2545F491, 0x4F6CDD1D };

	return MUL(rng_state, c).l;
}

float random()
{
	return xorshift64s() / 4294967296.0;
}

void initializeRandom(ULONG seed) {
	if (seed.h == 0 && seed.l == 0)
		seed.l = 1231231231;
	rng_state = seed;
}

