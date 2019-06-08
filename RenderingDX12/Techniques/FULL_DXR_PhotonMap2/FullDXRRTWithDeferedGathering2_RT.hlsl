#include "../CommonGI/Definitions.h"

// Photon Data
struct Photon {
	float3 Direction;
	float3 Intensity;
	float3 Position;
	float Radius;
};

#define IS_TRIANGLE_MASK 1
#define IS_PHOTON_MASK 2

// Top level structure with the scene
RaytracingAccelerationStructure Scene		: register(t0);

// > Photons buffer with photon information (position, direction, alpha and radius)
StructuredBuffer<Photon> Photons			: register(t1);

StructuredBuffer<Vertex> vertices			: register(t2);
StructuredBuffer<Material> materials		: register(t3);

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions					: register(t4);
Texture2D<float3> Normals					: register(t5);
Texture2D<float2> Coordinates				: register(t6);
Texture2D<int> MaterialIndices				: register(t7);
// Used for direct light visibility test
Texture2D<float3> LightPositions			: register(t8);

// Textures
Texture2D<float4> Textures[500]				: register(t9);

// Raytracing radiance (missing diffuse and specular interreflections)
RWTexture2D<float3> Output					: register(u0);

// Used for texture mapping
SamplerState gSmp : register(s0);
// Used for shadow map mapping
SamplerState shadowSmp : register(s1);

// Global constant buffer with view to world transform matrix
cbuffer Camera : register(b0) {
	// Light Space (View) to world space transform
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
// Locals for hit groups
ConstantBuffer<ObjInfo> objectInfo : register(b3);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct PhotonHitAttributes {
	// Photon Index
	int PhotonIdx;
};

struct RayPayload
{
	float3 color;
	int bounce;
};

struct PhotonRayPayload
{
	float3 InNormal;
	float InSpecularSharpness;
	float3 OutDiffuseAccum;
	float3 OutSpecularAccum;
};

#include "../CommonGI/ScatteringTools.h"

[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in PhotonHitAttributes attr) {
	float3 surfelPosition = WorldRayOrigin() + WorldRayDirection()*0.5;
	
	Photon p = Photons[attr.PhotonIdx];
	float3 V = WorldRayDirection();
	float3 H = normalize(V - p.Direction);
	float area = pi * p.Radius * p.Radius;

	float d = distance(surfelPosition, p.Position);

	if (d < p.Radius)
	{
		float kernel = 2 - 2 * d / p.Radius;
		payload.OutDiffuseAccum += p.Intensity * kernel / area;
		payload.OutSpecularAccum += p.Intensity* kernel*pow(saturate(dot(payload.InNormal, H)), payload.InSpecularSharpness) / area;
	}
	IgnoreHit(); // Continue search to accumulate other photons
}

[shader("intersection")]
void PhotonGatheringIntersection() {
	// use object info instead of PrimitiveIndex because
	// fallback device has errors for this function with procedural geometries
	int index = PrimitiveIndex();// objectInfo.MaterialIndex;
	PhotonHitAttributes att;
	att.PhotonIdx = index;
	ReportHit(0.001, 0, att);
}

[shader("miss")]
void PhotonGatheringMiss(inout PhotonRayPayload payload)
{
}

// Perform photon gathering using DXR API
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {

	PhotonRayPayload photonGatherPayload = {
		/*InNormal*/				surfel.N,
		/*InSpecularSharpness*/		material.SpecularSharpness,
		/*OutDiffuseAccum*/			float3(0,0,0),
		/*OutSpecularAccum*/		float3(0,0,0)
	};
	RayDesc ray;
	float3 dir = normalize(float3(1, 1, 1));
	ray.Origin = surfel.P - dir * 0.001;
	ray.Direction = dir * 0.002;
	ray.TMin = 0.0001;
	ray.TMax = 1;
	// Photon Map trace
	// PhotonMap ADS
	// RAY_FLAG_FORCE_NON_OPAQUE to produce any hit execution
	// IS_PHOTON_MASK : only photons are considered
	// 0 : Ray contribution to hitgroup index
	// 1 : Multiplier (all geometries AABBs will use the a hit group entry with the index because the bug of PrimitiveIndex)
	// 1 : Miss index for PhotonGatheringMiss shader
	// ray
	// raypayload
	TraceRay(Scene, RAY_FLAG_FORCE_NON_OPAQUE, IS_PHOTON_MASK, 0, 0, 1, ray, photonGatherPayload);
	return material.Diffuse / pi * photonGatherPayload.OutDiffuseAccum / 100000;// +material.Specular * photonGatherPayload.OutSpecularAccum;
}

float LightSphereRadius() {
	return 0.1;
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
			TraceRay(Scene, RAY_FLAG_NONE, IS_TRIANGLE_MASK, 0, 1, 0, reflectionRay, reflectionPayload);
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
			TraceRay(Scene, RAY_FLAG_NONE, IS_TRIANGLE_MASK, 0, 1, 0, refractionRay, refractionPayload);
			total += T.w * material.Specular * refractionPayload.color;
		}
	}
	return total;
}


[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
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
		Output[DispatchRaysIndex().xy] = float4(0, 0, 0, 1);
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
	Output[DispatchRaysIndex().xy] = RaytracingScattering(V, surfel, material, 2);
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
void RTScattering(inout RayPayload payload, in MyAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	float3 V = -WorldRayDirection();

	payload.color = RaytracingScattering(V, surfel, material, payload.bounce);
}
