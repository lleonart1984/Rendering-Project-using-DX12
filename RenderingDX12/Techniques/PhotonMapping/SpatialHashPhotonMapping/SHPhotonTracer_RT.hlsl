#define GRID_HASH_TABLE_REG			u1
#define LINKED_LIST_NEXT_REG		u2

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../CommonTracingSpatialHashGrid.hlsl.h"
#include "SHPMCommon.h" // Implements GetHashIndex method as a Teschner hashing (2003)

/// Nothing relevant during photon scattering
void OnPhotonScatter(float3 direction, Vertex surfel, Material material, float3 ratio, float pdf, float3 scatterDir, inout PTPayload payload)
{
}

/// Nothing relevant during payload scattering
void InitializePayload(uint2 photonRay, uint2 emissionImageDimensions, float3 emissionPoint, inout PTPayload payload)
{
}