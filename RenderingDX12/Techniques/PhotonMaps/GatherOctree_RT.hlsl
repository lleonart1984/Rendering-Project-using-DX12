// Override TEXTURES_REG
#define TEXTURES_REG t14

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "CommongPhotonGather_RT.hlsl.h"

StructuredBuffer<int> Morton		: register(t10);
StructuredBuffer<int> Permutation	: register(t11);
StructuredBuffer<int> LOD			: register(t12);
StructuredBuffer<int> Skip			: register(t13);

float squared(float v) { return v * v; }

bool intersect(float3 C1, float3 C2, float3 S, float R)
{
	float dist_squared = R * R;
	/* assume C1 and C2 are element-wise sorted, if not, do that now */
	if (S.x < C1.x) dist_squared -= squared(S.x - C1.x);
	else if (S.x > C2.x) dist_squared -= squared(S.x - C2.x);
	if (S.y < C1.y) dist_squared -= squared(S.y - C1.y);
	else if (S.y > C2.y) dist_squared -= squared(S.y - C2.y);
	if (S.z < C1.z) dist_squared -= squared(S.z - C1.z);
	else if (S.z > C2.z) dist_squared -= squared(S.z - C2.z);
	return dist_squared > 0;
}

int FromMortonToValueX(int morton) {
	int ans = morton & 0x09249249;
	ans = (ans | ans >> 2) & 0xc30c30c3;
	ans = (ans | ans >> 4) & 0x0f00f00f;
	ans = (ans | ans >> 8) & 0xff0000ff;
	ans = (ans | ans >> 16) & 0x000003FF;
	return ans;
}

int3 FromMortonToCell(int value) {
	return int3(
		FromMortonToValueX(value),
		FromMortonToValueX(value >> 1),
		FromMortonToValueX(value >> 2));
}

void GetBoxForMorton(int mortonIndex, int lod, out float3 c1, out float3 c2)
{
	mortonIndex = (mortonIndex >> (3 * lod)) << (3 * lod);

	int3 cell = FromMortonToCell(mortonIndex);
	c1 = cell / (float)(1 << 10) * 2 - 1;
	c2 = c1 + (2.0 / (1 << 10)) * (1 << lod);
}

void GetBoxForPosition(float3 P, int lod, out float3 c1, out float3 c2)
{
	int3 cell = ((int3)((P * 0.5 + 0.5) * 1024)) >> lod << lod;
	c1 = cell / (float)(1 << 10) * 2 - 1;
	c2 = c1 + (2.0 / (1 << 10)) * (1 << lod);
}

float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	float radius = PHOTON_RADIUS;
	float3 totalLighting = material.Emissive;

	//return surfel.P * 0.5 + 0.5;

	int currentPhoton = 0;
	int tries = 100000;
	while (currentPhoton < PHOTON_DIMENSION * PHOTON_DIMENSION)// && tries-- > 0)
	{
		int photonIndex = Permutation[currentPhoton];
		// Analyze photon and box
		Photon p = Photons[photonIndex];

		float NdotL = dot(surfel.N, -p.Direction);
		float NdotN = dot(surfel.N, p.Normal);

		float photonDistance = length(p.Position - surfel.P);
		float3 c1, c2;
		//GetBoxForMorton(Morton[photonIndex], LOD[currentPhoton], c1, c2);
		//GetBoxForPosition(p.Position, LOD[currentPhoton], c1, c2);
		GetBoxForPosition(p.Position, LOD[currentPhoton], c1, c2);
		bool inter = intersect(c1, c2, surfel.P, PHOTON_RADIUS);

		// Aggregate current Photon contribution if inside radius
		if (inter && photonDistance < radius && NdotL > 0.001 && NdotN > 0.001)
		{
			// Lambert Diffuse component (normalized dividing by pi)
			float3 DiffuseRatio = DiffuseBRDF(V, -p.Direction, surfel.N, NdotL, material);
			// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
			//float3 SpecularRatio = SpecularBRDF(V, -p.Direction, surfel.N, NdotL, material);

			float kernel = (1 - photonDistance / radius);

			float3 BRDF =
				NdotN ;// *DiffuseRatio;
				//+ material.Roulette.y * SpecularRatio;

			totalLighting += kernel * p.Intensity * BRDF;
		}
		
		// Move to the next photon
		currentPhoton = inter ? currentPhoton + 1 : Skip[currentPhoton];
	}

	return 2 * totalLighting * material.Roulette.x * material.Diffuse / pi / (100000 * pi * radius * radius);
}
