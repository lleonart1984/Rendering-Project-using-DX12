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
StructuredBuffer<int> Morton			: register (t1);

RWStructuredBuffer<AABB> PhotonAABBs	: register (u0);
RWStructuredBuffer<float> radii			: register (u1);

#define T 19

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

float gaussRandom() {
	return sqrt(-2.0 * log(random())*cos(3.14159 * 2 * random()));
}

float manhattanDistance(float3 p)
{
	p = abs(p);
	return max(p.x, max(p.y, p.z));
}

float MortonEstimator(in Photon currentPhoton, int index) {
	int l = index - 1;
	int r = index + 1;

	for (int i = 0; i < 10; i++) {
		int currentMask = ~((1 << (i * 3)) - 1);
		int currentBlock = Morton[index] & currentMask;
		float mortonBlockRadius = (1 << i) / 1024.0;
		
		// expand l and r considering all photons inside current block (currentMask)
		while (l >= 0 && ((Morton[l] & currentMask) == currentBlock))
			l--;
		while (r < PHOTON_DIMENSION*PHOTON_DIMENSION - 1 && any(Photons[r].Intensity) && ((Morton[r] & currentMask) == currentBlock))
			r++;
		if ((r - l) >= DESIRED_PHOTONS*4/3.14159)
		{
			return pow(mortonBlockRadius * mortonBlockRadius*DESIRED_PHOTONS / (r - l) * 4 / 3.14, 1.0/2);
		}
		if (mortonBlockRadius >= PHOTON_RADIUS)
			return PHOTON_RADIUS;
		
		//return mortonBlockRadius * 1.73 / pow((r - l)/ (float)DESIRED_PHOTONS, 0.5);
	}
	return PHOTON_RADIUS;
}


float HistogramEstimator2(in Photon currentPhoton, int index) {

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
float HistogramEstimator(in Photon currentPhoton, int index) {
	float histogram[T];
	for (int i = 0; i < T; i++)
		histogram[i] = 0;

	histogram[0]++; // same photon
	float3 minBox = 0;
	float3 maxBox = 0;
	int taken = 1;

	int l = index - 1;
	int r = index + 1;
	bool canTakeL, canTakeR;
	canTakeL = l >= 0;
	canTakeR = r < PHOTON_DIMENSION*PHOTON_DIMENSION - 1 && any(Photons[r].Intensity);

	int pTaken; float dTaken; float mTaken; float maxMTaken = 0;
	do {
		float dL = manhattanDistance(Photons[l].Position - currentPhoton.Position);
		float dR = manhattanDistance(Photons[r].Position - currentPhoton.Position);

		if (canTakeL && (!canTakeR || dL < dR))
		{
			pTaken = l;
			dTaken = dL;
			l--;
			canTakeL = l >= 0;
		}
		else
		{
			pTaken = r;
			dTaken = dR;
			r++;
			canTakeR = (r < PHOTON_DIMENSION*PHOTON_DIMENSION - 1) && any(Photons[r].Intensity);
		}
		taken += (dTaken < PHOTON_RADIUS);
		float3 vec = Photons[pTaken].Position - currentPhoton.Position;
		mTaken = manhattanDistance(vec);
		maxMTaken = min(PHOTON_RADIUS, max(mTaken, maxMTaken));
		minBox = min(minBox, vec);
		maxBox = max(maxBox, vec);

		int idx = max(0, min(T - 1, (int)(T * (dTaken / PHOTON_RADIUS))));
		histogram[idx] += (dTaken < PHOTON_RADIUS);

	} while (
		((mTaken <= PHOTON_RADIUS && taken < DESIRED_PHOTONS) ||
		(mTaken <= maxMTaken))
		&& (canTakeL || canTakeR));

	float boxR = max(
			max(maxBox.x, -minBox.x),
		max(max(maxBox.y, -minBox.y),
			max(maxBox.z, -minBox.z))
	);

	float sx = clamp(max(maxBox.x, -minBox.x)/(maxBox.x - minBox.x), 1, 2);
	float sy = clamp(max(maxBox.y, -minBox.y)/(maxBox.y - minBox.y), 1, 2);
	float sz = clamp(max(maxBox.z, -minBox.z)/(maxBox.z - minBox.z), 1, 2);

	float samplingScaling = 1;// pow(sx * sy * sz, 2.0 / 3);

	float counting = DESIRED_PHOTONS / samplingScaling;
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
		radius = clamp(MortonEstimator(currentPhoton, index), PHOTON_RADIUS*0.001, PHOTON_RADIUS);

		box.minimum = currentPhoton.Position - radius;
		box.maximum = currentPhoton.Position + radius;
	}
	else {
		radius = 0;

		float x = raysIndex.x / (float)PHOTON_DIMENSION;
		float y = raysIndex.y / (float)PHOTON_DIMENSION;
		float z = 2 * (abs(index * 0x888888) % 128) / 128.0 - 1;

		float3 pos = 0;// 2 * float3(x, y, z) - 1;
		//float3 pos = Photons[0].Position;

		box.minimum = pos - 0.00001;// -radius * 0.0001;
		box.maximum = pos + 0.00001;// +radius * 0.0001;
	}
	radii[index] = radius;
	PhotonAABBs[index] = box;
}