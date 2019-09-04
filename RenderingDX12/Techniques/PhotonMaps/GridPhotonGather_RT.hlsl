// Override TEXTURES_REG
#define TEXTURES_REG t11

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "CommongPhotonGather_RT.hlsl.h"
#include "CommonHashing_RT.hlsl.h"

StructuredBuffer<int> HashTableBuffer	: register(t9);
StructuredBuffer<int> NextBuffer		: register(t10);

#define T 20

#ifndef DEBUG_PHOTONS
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	float radius = PHOTON_RADIUS;
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
}
#else
#if DEBUG_STRATEGY == 0
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	float radius = PHOTON_RADIUS;
	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);

	float3 counting = 0;

	for (int dz = begCell.z; dz <= endCell.z; dz++)
		for (int dy = begCell.y; dy <= endCell.y; dy++)
			for (int dx = begCell.x; dx <= endCell.x; dx++)
			{
				int cellIndexToQuery = GetHashIndex(int3(dx, dy, dz));

				if (cellIndexToQuery != -1) // valid coordinates
				{
					int currentPhotonPtr = HashTableBuffer[cellIndexToQuery];

					while (currentPhotonPtr != -1) {

						counting += 1;

						currentPhotonPtr = NextBuffer[currentPhotonPtr];
					}
				}
			}

	return counting;
}
#else
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	float radius = PHOTON_RADIUS;
	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);
	int histogram[T];
	for (int i = 0; i < T; i++)
		histogram[i] = 0;


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

						float d = distance(p.Position, surfel.P);

						if (d < PHOTON_RADIUS)
						{
							int index = min(T - 1, T * d / PHOTON_RADIUS);
							histogram[index]++;
						}

						currentPhotonPtr = NextBuffer[currentPhotonPtr];
					}
				}
			}

	int limit = 200;
	int selected = 0;
	int counting = 0;
	for (int i = 0; i < T; i++)
	{
		counting += histogram[i];
		if (counting > limit)
		{
			selected = i;
			break;
		}
	}

	float radiusRatio = selected / (float)T + (1.0 / T)*(counting - histogram[selected]) / (float)histogram[selected];

	return pow(4, radiusRatio * 7);
}

#endif
#endif