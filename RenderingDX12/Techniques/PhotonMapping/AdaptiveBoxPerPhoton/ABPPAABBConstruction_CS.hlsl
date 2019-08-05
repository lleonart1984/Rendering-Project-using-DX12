
#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../PhotonDefinition.h"

struct AABB {
	float3 minimum;
	float3 maximum;
};

StructuredBuffer<Photon> Photons	: register (t0);
StructuredBuffer<float> radii		: register (t1);

RWStructuredBuffer<AABB> AABBs		: register (u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;
	
	float radius = radii[index];
	Photon p = Photons[index];

	if (!any(p.Intensity))
		radius = 0;

	AABB result;
	result.minimum = p.Position - radius;
	result.maximum = p.Position + radius;
	AABBs[index] = result;
}