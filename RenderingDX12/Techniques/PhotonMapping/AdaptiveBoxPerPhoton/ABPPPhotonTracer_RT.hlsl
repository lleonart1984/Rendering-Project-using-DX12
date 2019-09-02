
#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../CommonDeferredTracing.hlsl.h"

/// Nothing relevant during photon adding
void OnAddPhoton(inout Photon photon, Vertex surfel, PTPayload payload, int photonIndex) {
}

void StorePhoton(Photon photon, int photonIndex) {
}

/// Nothing relevant during photon scattering
void OnPhotonScatter(float3 direction, Vertex surfel, Material material, float3 ratio, float pdf, float3 scatterDir, inout PTPayload payload)
{
}

/// Nothing relevant during payload initialization
void InitializePayload(uint2 photonRay, uint2 emissionImageDimensions, float3 emissionPoint, inout PTPayload payload)
{
}