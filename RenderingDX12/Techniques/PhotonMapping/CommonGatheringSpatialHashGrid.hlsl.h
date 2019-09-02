StructuredBuffer<int> HashTableBuffer	: register(GRID_HASH_TABLE_REG);
StructuredBuffer<int> NextBuffer		: register(LINKED_LIST_NEXT_REG);

#include "CommonDeferredGathering.hlsl.h"
#include "CommonSpatialHashing.hlsl.h"

StructuredBuffer<Photon> Photons		: register(PHOTON_BUFFER_REG);

float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
#ifdef PHOTON_WITH_RADIUS
	float radius = p.Radius;
#else
	float radius = PHOTON_RADIUS;
#endif

#ifndef DEBUG_PHOTONS
	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);

	float3 totalLighting = material.Emissive;

	for (int dz = begCell.z; dz <= endCell.z; dz++)
		for (int dy = begCell.y; dy <= endCell.y; dy++)
			for (int dx = begCell.x; dx <= endCell.x; dx++)
			{
				int cellIndexToQuery = GetHashIndex(int3(dx, dy, dz));

				if (cellIndexToQuery != -1) // valid coordinates
				{
					int currentPhotonPtr = HashTableBuffer[cellIndexToQuery];

					while (currentPhotonPtr != -1) {

						Photon p = Photons[currentPhotonPtr];

						float NdotL = dot(surfel.N, -p.Direction);
						float NdotN = dot(surfel.N, p.Normal);

						float photonDistance = length(p.Position - surfel.P);

						// Aggregate current Photon contribution if inside radius
						if (photonDistance < radius && NdotL > 0.001 && NdotN > 0.001)
						{
							// Lambert Diffuse component (normalized dividing by pi)
							float3 DiffuseRatio = DiffuseBRDF(V, -p.Direction, surfel.N, NdotL, material);
							// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
							//float3 SpecularRatio = SpecularBRDF(V, -p.Direction, surfel.N, NdotL, material);

							float kernel = 2 * (1 - photonDistance / radius);

							float3 BRDF =
								NdotN * material.Roulette.x * material.Diffuse / pi;// *DiffuseRatio;
								//+ material.Roulette.y * SpecularRatio;

							totalLighting += kernel * p.Intensity * BRDF;
						}

						currentPhotonPtr = NextBuffer[currentPhotonPtr];
					}
				}
			}

	return totalLighting / (100000 * pi * radius * radius);
#else
	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);

	float3 totalLighting = 0;

	for (int dz = begCell.z; dz <= endCell.z; dz++)
		for (int dy = begCell.y; dy <= endCell.y; dy++)
			for (int dx = begCell.x; dx <= endCell.x; dx++)
			{
				int cellIndexToQuery = GetHashIndex(int3(dx, dy, dz));

				if (cellIndexToQuery != -1) // valid coordinates
				{
					int currentPhotonPtr = HashTableBuffer[cellIndexToQuery];

					while (currentPhotonPtr != -1) {

						Photon p = Photons[currentPhotonPtr];

						if (distance(p.Position, surfel.P) < PHOTON_RADIUS)
							totalLighting += 1;
						
						currentPhotonPtr = NextBuffer[currentPhotonPtr];
					}
				}
			}

	return totalLighting;
#endif
}


float3 ComputeDirectLightInWorldSpace2(Vertex surfel, Material material, float3 V) {
	float radius = PHOTON_RADIUS;// 4 * max(CellSize.x, max(CellSize.y, CellSize.z));
	int intRadius = (int)ceil(PHOTON_RADIUS / PHOTON_CELL_SIZE);

	float3 totalLighting = 0;

	int3 centerCell = FromPositionToCell(surfel.P);

	float maxDistance = 0;

	int count = 0;
	int r = 0;
	while (r <= intRadius && count < 20) {
		for (int dx = -r; dx <= r; dx++)
		{
			int yR = r - abs(dx);
			for (int dy = -yR; dy <= yR; dy++)
			{
				int zR = yR - abs(dy);
				for (int dz = -zR; dz <= zR; dz += max(1, zR) * 2)
				{
					int cellIndexToQuery = GetHashIndex(centerCell + int3(dx, dy, dz));

					if (cellIndexToQuery != -1) // valid coordinates
					{
						int currentPhotonPtr = HashTableBuffer[cellIndexToQuery];

						while (currentPhotonPtr != -1) {

							Photon p = Photons[currentPhotonPtr];

							float NdotL = dot(surfel.N, -p.Direction);
							float NdotN = dot(surfel.N, p.Normal);

							float photonDistance = length(p.Position - surfel.P);

							// Aggregate current Photon contribution if inside radius
							if (photonDistance < radius && NdotL > 0.001 && NdotN > 0.001)
							{
								maxDistance = max(maxDistance, photonDistance);
								// Lambert Diffuse component (normalized dividing by pi)
								float3 DiffuseRatio = DiffuseBRDF(V, -p.Direction, surfel.N, NdotL, material);
								// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
								//float3 SpecularRatio = SpecularBRDF(V, -p.Direction, surfel.N, NdotL, material);

								float kernel = 2 * (1 - photonDistance / radius);

								float3 BRDF =
									NdotN * material.Roulette.x * material.Diffuse / pi;// *DiffuseRatio;
									//+ material.Roulette.y * SpecularRatio;

								totalLighting += kernel * p.Intensity * BRDF;

								count++;
							}

							currentPhotonPtr = NextBuffer[currentPhotonPtr];
						}
					}
				}
			}
		}

		r++;
	}

	if (count >= 20)
		radius = maxDistance;

	return totalLighting / (100000 * pi * radius * radius);
}