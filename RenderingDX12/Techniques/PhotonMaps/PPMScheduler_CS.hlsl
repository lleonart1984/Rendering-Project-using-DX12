#include "../CommonGI/Parameters.h"
#include "../CommonGI/Definitions.h"
#include "CommonHashing_RT.h"

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"

struct RayHitInfo {
	Vertex Surfel;
	float2 Grad;
	float3 D; // Direction
	float3 I; // Importance
	int MI; // Material index
};

StructuredBuffer<RayHitInfo> RTHits		: register(t0);
StructuredBuffer<Photon> Photons		: register(t1);
StructuredBuffer<float> Radii			: register(t2);

Texture2D<int> ScreenHead				: register(t3);
StructuredBuffer<int> ScreenNext		: register(t4);

StructuredBuffer<int> GridHead			: register(t5);
StructuredBuffer<int> GridNext			: register(t6);

struct RayHitAccum {
	float N;
	float3 Accum;
};

RWStructuredBuffer<RayHitAccum> PM : register (u0);

void AccumulatePhotons(int currentHit) {
	int N = 0;
	float3 Accum = 0;

	RayHitInfo info = RTHits[currentHit];

	float radius = Radii[currentHit];

	int3 begCell = FromPositionToCell(info.Surfel.P - radius);
	int3 endCell = FromPositionToCell(info.Surfel.P + radius);

	for (int z = begCell.z; z <= endCell.z; z++)
		for (int y = begCell.y; y <= endCell.y; y++)
			for (int x = begCell.x; x <= endCell.x; x++) {

				int3 currentCell = int3(x, y, z);

				int hashIndex = GetHashIndex(currentCell);

				int currentPhoton = GridHead[hashIndex];

				while (currentPhoton != -1) {

					Photon p = Photons[currentPhoton];
					float photonDistance = length(p.Position - info.Surfel.P);

					if (photonDistance < radius) 
					{
						float NdotL = dot(info.Surfel.N, -p.Direction);
						float NdotN = dot(info.Surfel.N, p.Normal);
						float NdotV = dot(info.D, p.Normal);

						float kernel = (NdotV > 0.0001)* (NdotL > 0.001)* max(0, 1 - (photonDistance / radius));

						// Add photon to accum
						Accum += kernel * p.Intensity * max(0, NdotN);
						N++;
					}

					currentPhoton = GridNext[currentPhoton];
				}
			}

	PM[currentHit].N = N;
	PM[currentHit].Accum = Accum;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int2 dim;
	ScreenHead.GetDimensions(dim.x, dim.y);

	int2 px = DTid.xy;

	if (!all(px < dim))
		return;

	int currentHit = ScreenHead[px];

	while (currentHit != -1) {

		AccumulatePhotons(currentHit);

		currentHit = ScreenNext[currentHit];
	}
}