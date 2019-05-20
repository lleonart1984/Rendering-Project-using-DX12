#include "../CommonGI/Definitions.h"

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
// Textures
Texture2D<float4> Textures[500]			: register(t8);

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

cbuffer PathtracingInfo : register(b3) {
	int CurrentPass;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b4);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct Ray {
	float3 Position;
	float3 Direction;
};

struct RayPayload
{
	float3 Importance;
	float3 AccRadiance;
	Ray ScatteredRay;
};

#include "../CommonGI/ScatteringTools.h"

// Overriden Shadow cast computation for direct light computation
// Gets true if current surfel is lit by the light source
// checking not with DXR but with classic shadow maps represented
// by GBuffer obtained from light
float ShadowCast(Vertex surfel)
{
	float3 pInLightViewSpace = mul(float4(surfel.P, 1), LightView).xyz;
	float4 pInLightProjSpace = mul(float4(pInLightViewSpace, 1), LightProj);
	if (pInLightProjSpace.z <= 0.01)
		return 0;
	pInLightProjSpace.xyz /= pInLightProjSpace.w;
	float2 cToTest = 0.5 + 0.5 * pInLightProjSpace.xy;
	cToTest.y = 1 - cToTest.y;
	float3 lightSampleP = LightPositions.SampleGrad(shadowSmp, cToTest, 0, 0);
	return pInLightViewSpace.z - lightSampleP.z < 0.001 ? 1 : 0;
}

// Overriden light sphere radius for direct light computation
float LightSphereRadius() {
	return 0.1;
}

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void SurfelScattering(float3 V, Vertex surfel, Material material, inout RayPayload payload)
{
	// Adding emissive and direct lighting
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;
	// Update Accumulated Radiance to the viewer
	payload.AccRadiance += payload.Importance *
		(material.Emissive 
			// Next Event estimation
			+ ComputeDirectLighting(V, surfel, material, LightPosition, LightIntensity,
				// Co-lateral outputs
				NdotV, invertNormal, fN, R, T));

	float3 ratio;
	float3 direction;

	RandomScatterRay(V, fN, R, T, material, ratio, direction);

	// Update gathered Importance to the viewer
	payload.Importance *= ratio;// / (1 - russianRoulette);
	// Update scattered ray
	payload.ScatteredRay.Direction = direction;
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN))*0.001*fN;
}

float3 ComputePath(float3 V, Vertex surfel, Material material, int bounces)
{
	RayPayload payload = (RayPayload)0;
	payload.Importance = 1;

	// initial scatter (primary rays)
	SurfelScattering(V, surfel, material, payload);

	[loop]
	for (int bounce = 0; bounce < bounces; bounce++)
	{
		RayDesc newRay;
		newRay.Origin = payload.ScatteredRay.Position;
		newRay.Direction = payload.ScatteredRay.Direction;
		newRay.TMin = 0.001;
		newRay.TMax = 10000.0;

		if (any(payload.Importance > 0.01))
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
	}

	return payload.AccRadiance;
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
}

#define PATHS_PER_PASS 10

[shader("raygeneration")]
void PTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	// Initialize random iterator using rays index as seed
	initializeRandom(raysIndex.x + raysIndex.y*raysDimensions.x + raysDimensions.x*raysDimensions.y*CurrentPass);

	float3 P = Positions[raysIndex];
	float3 N = Normals[raysIndex];
	float2 C = Coordinates[raysIndex];
	Material material = materials[MaterialIndices[raysIndex]];

	if (!any(P)) // force miss execution
	{
		Output[DispatchRaysIndex().xy] += float4(0,0,0, 1);
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

	float3 totalAcc = 0;

	[loop]
	for (int i = 0; i < PATHS_PER_PASS; i++)
		totalAcc += ComputePath(V, surfel, material, 2);
	// Write the raytraced color to the output texture.
	Output[DispatchRaysIndex().xy] = (Output[DispatchRaysIndex().xy] * PATHS_PER_PASS * CurrentPass + totalAcc) / (PATHS_PER_PASS * (CurrentPass + 1));
}

void GetHitInfo(in MyAttributes attr, out Vertex surfel, out Material material)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	uint triangleIndex = objectInfo.TriangleOffset + PrimitiveIndex();
	uint materialIndex = objectInfo.MaterialIndex;

	Vertex v1 = vertices[triangleIndex * 3 + 0];
	Vertex v2 = vertices[triangleIndex * 3 + 1];
	Vertex v3 = vertices[triangleIndex * 3 + 2];
	Vertex s = {
		v1.P * barycentrics.x + v2.P * barycentrics.y + v3.P * barycentrics.z,
		v1.N * barycentrics.x + v2.N * barycentrics.y + v3.N * barycentrics.z,
		v1.C * barycentrics.x + v2.C * barycentrics.y + v3.C * barycentrics.z,
		v1.T * barycentrics.x + v2.T * barycentrics.y + v3.T * barycentrics.z,
		v1.B * barycentrics.x + v2.B * barycentrics.y + v3.B * barycentrics.z
	};

	surfel = Transform(s, ObjectToWorld4x3());

	material = materials[materialIndex];

	AugmentHitInfoWithTextureMapping(surfel, material);
}

[shader("closesthit")]
void PTScattering(inout RayPayload payload, in MyAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	// This is not a recursive closest hit but it will accumulate in payload
	// all the result of the scattering to this surface
	SurfelScattering(-WorldRayDirection(), surfel, material, payload);
}
