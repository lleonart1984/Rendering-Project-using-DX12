
// Morton indice for each photon. Used as key to sort.
StructuredBuffer<int> Indices			: register(t0);
// Permutation of the sorting.
RWStructuredBuffer<int> Permutation		: register (u0);

cbuffer BitonicStage : register(b0) {
	int len;
	int dif;
};

[shader("raygeneration")]
void Main()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int index = raysIndex.y * raysDimensions.x + raysIndex.x;

	int mask = dif - 1;

	int i = (index & mask) | ((index & ~mask) << 1);
	int next = i | dif;

	bool dec = Indices[Permutation[i]] > Indices[Permutation[next]];
	bool shouldDec = (i & len) != 0;
	bool swap = ((!shouldDec && dec) || (shouldDec && !dec));

	if (swap) {
		InterlockedExchange(Permutation[i], Permutation[next], Permutation[next]);
	}

	//int toI = swap ? Permutation[next] : Permutation[i];
	//int toN = swap ? Permutation[i] : Permutation[next];

	//Permutation[i] = toI;
	//Permutation[next] = toN;
}