
#ifndef GEOMETRY_HASH_TABLE_REG
#define GEOMETRY_HASH_TABLE_REG u1
#endif

#ifndef LINKED_LIST_NEXT_REG
#define LINKED_LIST_NEXT_REG u2
#endif

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "../CommonDeferredTracing.hlsl.h"

RWStructuredBuffer<int> HashTableBuffer	: register(GEOMETRY_HASH_TABLE_REG);
RWStructuredBuffer<int> NextBuffer		: register(LINKED_LIST_NEXT_REG);

void OnAddPhoton(inout Photon photon, Vertex surfel, PTPayload payload, int photonIndex) {
	int cellIndex = PrimitiveIndex();
	InterlockedExchange(HashTableBuffer[cellIndex], photonIndex, NextBuffer[photonIndex]); // Update head and next reference
}

/// Nothing relevant during photon scattering
void OnPhotonScatter(float3 direction, Vertex surfel, Material material, float3 ratio, float pdf, float3 scatterDir, inout PTPayload payload)
{
}

/// Nothing relevant during payload initialization
void InitializePayload(uint2 photonRay, uint2 emissionImageDimensions, float3 emissionPoint, inout PTPayload payload)
{
}