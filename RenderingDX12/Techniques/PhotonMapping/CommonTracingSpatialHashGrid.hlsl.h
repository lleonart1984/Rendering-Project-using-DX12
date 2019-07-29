RWStructuredBuffer<int> HashTableBuffer	: register(GRID_HASH_TABLE_REG);
RWStructuredBuffer<int> NextBuffer		: register(LINKED_LIST_NEXT_REG);

#include "CommonDeferredTracing.hlsl.h"
#include "CommonSpatialHashing.hlsl.h"

void OnAddPhoton(inout Photon photon, Vertex surfel, PTPayload payload, int photonIndex) {
	int cellIndex = GetHashIndex(surfel.P);
	InterlockedExchange(HashTableBuffer[cellIndex], photonIndex, NextBuffer[photonIndex]); // Update head and next reference
}