#include "../../CommonGI/Definitions.h"
#include "../../CommonGI/Parameters.h"

// Photon Data
struct Photon {
	float3 Position;
	float3 Direction;
	float3 Intensity;
};

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);
StructuredBuffer<Vertex> vertices		: register(t1);
StructuredBuffer<Material> materials	: register(t2);

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(t3);
Texture2D<float3> Normals				: register(t4);
Texture2D<float2> Coordinates			: register(t5);
Texture2D<int> MaterialIndices			: register(t6);
// Used for direct light visibility test
Texture2D<float3> LightPositions		: register(t7);
// Photon Map binding objects
StructuredBuffer<int> HashTable			: register(t8);
StructuredBuffer<Photon> Photons		: register(t9);
StructuredBuffer<int> NextBuffer		: register(t10);

// Textures
Texture2D<float4> Textures[500]			: register(t11);

// Raytracing output image
RWTexture2D<float3> Output				: register(u0);

// Used for texture mapping
SamplerState gSmp : register(s0);
// Used for shadow map mapping
SamplerState shadowSmp : register(s1);

// Global constant buffer with view to world transform matrix
cbuffer Camera : register(b0) {
	row_major matrix ViewToWorld;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
}

// Matrices to transform from Light space (proj and view) to world space
cbuffer LightTransforms : register(b2) {
	row_major matrix LightProj;
	row_major matrix LightView;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b3);

struct RayPayload
{
	float3 color;
	int bounce;
};

#include "../../CommonGI/ScatteringTools.h"
#include "SHCommon.h"

// Perform photon gathering
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {

	float radius = PHOTON_RADIUS;

	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);

	float3 totalLighting = material.Emissive;

	for (int dz = begCell.z; dz <= endCell.z; dz++)
		for (int dy = begCell.y; dy <= endCell.y; dy++)
			for (int dx = begCell.x; dx <= endCell.x; dx++)
			{
				int cellIndexToQuery = FromCellToCellIndex(int3(dx, dy, dz));

				int currentPhotonPtr = HashTable[cellIndexToQuery];

				while (currentPhotonPtr != -1) {

					Photon p = Photons[currentPhotonPtr];

					float NdotL = dot(surfel.N, -p.Direction);

					float photonDistance = length(p.Position - surfel.P);

					// Aggregate current Photon contribution if inside radius
					if (photonDistance < radius && NdotL > 0.001)
					{
						// Lambert Diffuse component (normalized dividing by pi)
						float3 DiffuseRatio = DiffuseBRDF(V, -p.Direction, surfel.N, NdotL, material);
						// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
						//float3 SpecularRatio = SpecularBRDF(V, -p.Direction, surfel.N, NdotL, material);

						float kernel = 2 * (1 - photonDistance / radius);

						float3 BRDF =
							material.Roulette.x * material.Diffuse / pi;// *DiffuseRatio;
							//+ material.Roulette.y * SpecularRatio;

						totalLighting += NdotL * kernel * p.Intensity * BRDF;
					}

					currentPhotonPtr = NextBuffer[currentPhotonPtr];
				}
			}

	return totalLighting / (100000 * pi*radius*radius);
}

// Gets true if current surfel is lit by the light source
// checking not with DXR but with classic shadow maps represented
// by GBuffer obtained from light
float ShadowCast(Vertex surfel)
{
	float3 pInLightViewSpace = mul(float4(surfel.P, 1), LightView).xyz;
	float4 pInLightProjSpace = mul(float4(pInLightViewSpace, 1), LightProj);
	if (pInLightProjSpace.z <= 0)
		return 0;
	float2 cToTest = 0.5 + 0.5 * pInLightProjSpace.xy / pInLightProjSpace.w;
	cToTest.y = 1 - cToTest.y;
	float3 lightSampleP = LightPositions.SampleGrad(shadowSmp, cToTest, 0, 0);
	return pInLightViewSpace.z - lightSampleP.z < 0.001;
}

float3 RaytracingScattering(float3 V, Vertex surfel, Material material, int bounces)
{
	float3 total = float3(0, 0, 0);

	// Adding Emissive
	total += material.Emissive;

	// Adding direct lighting
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;
	total += ComputeDirectLighting(
		V,
		surfel,
		material,
		LightPosition,
		LightIntensity,
		/*Out*/ NdotV, invertNormal, fN, R, T);

	// Get indirect diffuse light compute using photon map
	total += ComputeDirectLightInWorldSpace(surfel, material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);

	if (bounces > 0)
	{
		if (R.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + fN * 0.0001;
			reflectionRay.Direction = R.xyz;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RayPayload reflectionPayload = { float3(0, 0, 0), bounces - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, reflectionRay, reflectionPayload);
			total += R.w * material.Specular * reflectionPayload.color; /// Mirror and fresnel reflection
		}

		if (T.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.0001;
			refractionRay.Direction = T.xyz;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RayPayload refractionPayload = { float3(0, 0, 0), bounces - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractionRay, refractionPayload);
			total += T.w * material.Specular * refractionPayload.color;
		}
	}
	return total;
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	//payload.color = WorldRayDirection();
}

[shader("raygeneration")]
void RTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	float3 P = Positions[raysIndex];
	float3 N = Normals[raysIndex];
	float2 C = Coordinates[raysIndex];
	Material material = materials[MaterialIndices[raysIndex]];

	if (!any(P)) // force miss execution
	{
		RayPayload payload = { float3(0,0,0), 0 };
		//EnvironmentMap(payload);
		Output[DispatchRaysIndex().xy] = float4(payload.color, 1);
		return;
	}

	float3 V = normalize(-P); // In view spce "viewer" is positioned in (0,0,0) 

	// Move to world space
	P = mul(float4(P, 1), ViewToWorld).xyz;
	N = mul(float4(N, 0), ViewToWorld).xyz;
	V = mul(float4(V, 0), ViewToWorld).xyz;

	Vertex surfel = {
		P,
		N,
		C,
		float3(0,0,0),
		float3(0,0,0)
	};

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material);

	// Write the raytraced color to the output texture.
	Output[DispatchRaysIndex().xy] = RaytracingScattering(V, surfel, material, RAY_TRACING_MAX_BOUNCES);
}



[shader("closesthit")]
void RTScattering(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	float3 V = -normalize(WorldRayDirection());

	payload.color = RaytracingScattering(V, surfel, material, payload.bounce);
}
