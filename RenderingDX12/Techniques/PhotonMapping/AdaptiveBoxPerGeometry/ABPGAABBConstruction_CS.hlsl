#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../PhotonDefinition.h"
#include "../../CommonGI/Parameters.h"

struct AABB {
	float3 minimum;
	float3 maximum;
};

StructuredBuffer<Photon> Photons	: register (t0);
StructuredBuffer<int> HashTable		: register (t1);
StructuredBuffer<int> NextBuffer	: register (t2);
StructuredBuffer<float> radii		: register (t3);

RWStructuredBuffer<AABB> AABBs		: register (u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	float3 minimum = 10000;
	float3 maximum = -10000;

	int currentIndex = HashTable[index];

	while (currentIndex != -1) {
		float3 position = Photons[index].Position;
		float radius = radii[index];
		minimum = min(minimum, position - radius);
		maximum = max(maximum, position + radius);

		currentIndex = NextBuffer[currentIndex];
	}

	AABBs[index].minimum = minimum.x >= maximum.x ? 0 : minimum;
	AABBs[index].maximum = minimum.x >= maximum.x ? 0 : maximum;
}