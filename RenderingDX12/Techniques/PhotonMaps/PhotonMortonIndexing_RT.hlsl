#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"
StructuredBuffer<Photon> Photons : register(t0);
RWStructuredBuffer<int> Indices : register (u0);

int split3(int value) {
	
	int ans = value & 0x3FF; // allow 10 bits only.

	ans = (ans | ans << 16) &	0xff0000ff;
	// poniendo 8 ceros entre cada grupo de 4 bits.
	// shift left 8 bits y despues hacer OR entre ans y 0001000000001111000000001111000000001111000000001111000000000000
	ans = (ans | ans << 8) &	0x0f00f00f;
	// poniendo 4 ceros entre cada grupo de 2 bits.
	// shift left 4 bits y despues hacer OR entre ans y 0001000011000011000011000011000011000011000011000011000011000011
	ans = (ans | ans << 4) &	0xc30c30c3;
	// poniendo 2 ceros entre cada bit.
	// shift left 2 bits y despues hacer OR entre ans y 0001001001001001001001001001001001001001001001001001001001001001
	ans = (ans | ans << 2) &	0x49249249;
	return ans;
}

int morton(int3 pos) {
	return split3(pos.x) | (split3(pos.y) << 1) | (split3(pos.z) << 2);
}

[shader("raygeneration")]
void Main()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int index = raysIndex.y * raysDimensions.x + raysIndex.x;

	int3 pos = saturate(Photons[index].Position*0.5 + 0.5) * ((1 << 10) - 1); // map to 1024^3 cells

	Indices[index] = any(Photons[index].Intensity) ? morton(pos) : 0x7FFFFFFF;
}