#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

struct AABB {
	float3 minimum;
	float3 maximum;
};

StructuredBuffer<Photon> Photons				: register (t0);
StructuredBuffer<float> Radii					: register (t1);

RWStructuredBuffer<AABB> Boxes					: register (u0);

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int index = DTid.x;

	AABB box = (AABB)0;
	box.minimum = 10000;
	box.maximum = -10000;

	for (int i = 0; i < BOXED_PHOTONS; i++)
	{
		int photonIdx = index * BOXED_PHOTONS + i;

		Photon currentPhoton = Photons[photonIdx];
		float radius = 0;

		if (any(currentPhoton.Intensity))
		{
			float radius = Radii[photonIdx];
			box.minimum = min(box.minimum, currentPhoton.Position - radius);
			box.maximum = max(box.maximum, currentPhoton.Position + radius);
		}
	}

	if (box.maximum.x <= box.minimum.x)
	{
		box.minimum = 0;
		box.maximum = 0;
	}

	Boxes[index] = box;
}