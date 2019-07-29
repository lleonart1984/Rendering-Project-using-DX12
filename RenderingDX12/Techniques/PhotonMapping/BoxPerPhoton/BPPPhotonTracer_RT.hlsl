
#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../CommonDeferredTracing.hlsl.h"

struct AABB {
	float3 minimum;
	float3 maximum;
};

#ifndef PHOTON_AABBS_REG
#define PHOTON_AABBS_REG u1
#endif

RWStructuredBuffer<AABB> PhotonAABBs : register(PHOTON_AABBS_REG);

void OnAddPhoton(inout Photon photon, Vertex surfel, PTPayload payload, int photonIndex) {

	AABB box = {
		surfel.P - PHOTON_RADIUS,
		surfel.P + PHOTON_RADIUS
	};

	PhotonAABBs[photonIndex] = box;
}

/// Nothing relevant during photon scattering
void OnPhotonScatter(float3 direction, Vertex surfel, Material material, float3 ratio, float pdf, float3 scatterDir, inout PTPayload payload)
{
}

/// Clear box relative to the photon during payload initialization
void InitializePayload(uint2 photonRay, uint2 emissionImageDimensions, float3 emissionPoint, inout PTPayload payload)
{
	int photonIndex = photonRay.x + photonRay.y * emissionImageDimensions.x;
	// Degenerate unused box
	PhotonAABBs[photonIndex] = (AABB)0;
}