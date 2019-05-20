#include "../CommonGI/Definitions.h"

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);
StructuredBuffer<Vertex> vertices		: register(t1);
StructuredBuffer<Material> materials	: register(t2);
Texture2D<float4> Textures[200]			: register(t3);

// Raytracing output image
RWTexture2D<float4> RenderTarget : register(u0);

SamplerState gSmp : register(s0);

// Global constant buffer with projection to world transform matrix
cbuffer Camera : register(b0) {
	row_major matrix ProjectionToWorld;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b2);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

#include "../CommonGI/ScatteringTools.h"

struct RayPayload
{
	float3 color;
	int bounce;
};

[shader("raygeneration")]
void MainRays()
{
	float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

	float2 screenXY = 2 * lerpValues - 1;
	screenXY.y *= -1;

	float4 rayBeg = mul(float4(screenXY, 0, 1), ProjectionToWorld);
	float4 rayEnd = mul(float4(screenXY, 1, 1), ProjectionToWorld);

	float3 origin = rayBeg.xyz / rayBeg.w;
	float3 rayDir = normalize(rayEnd.xyz / rayEnd.w - origin);

	// Trace the ray.
	// Set the ray's extents.
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	ray.TMin = 0.001;
	ray.TMax = 10000.0;
	RayPayload payload = { float3(0, 0, 0), 3 };
	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

	// Write the raytraced color to the output texture.
	RenderTarget[DispatchRaysIndex().xy] = float4(payload.color, 1);
}

[shader("closesthit")]
void FresnelScattering(inout RayPayload payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	uint triangleIndex = objectInfo.TriangleOffset + PrimitiveIndex();
	uint materialIndex = objectInfo.MaterialIndex;

	Vertex v1 = vertices[triangleIndex * 3 + 0];
	Vertex v2 = vertices[triangleIndex * 3 + 1];
	Vertex v3 = vertices[triangleIndex * 3 + 2];
	Vertex surfel = {
		v1.P * barycentrics.x + v2.P * barycentrics.y + v3.P * barycentrics.z,
		v1.N * barycentrics.x + v2.N * barycentrics.y + v3.N * barycentrics.z,
		v1.C * barycentrics.x + v2.C * barycentrics.y + v3.C * barycentrics.z,
		v1.T * barycentrics.x + v2.T * barycentrics.y + v3.T * barycentrics.z,
		v1.B * barycentrics.x + v2.B * barycentrics.y + v3.B * barycentrics.z
	};

	surfel = Transform(surfel, ObjectToWorld4x3());
	Material material = materials[materialIndex];

	AugmentHitInfoWithTextureMapping(surfel, material);

	float3 V = -WorldRayDirection();
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R;
	float4 T;
	float3 directLight = ComputeDirectLightingNoShadowCast(
		V, surfel, material, LightPosition, LightIntensity,
		NdotV,
		invertNormal,
		fN,
		R,
		T
	);

	float3 totalLight = material.Emissive + directLight;

	if (payload.bounce > 0)
	{
		if (R.w > 0) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + fN * 0.001;
			reflectionRay.Direction = R.xyz;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RayPayload reflectionPayload = { float3(0, 0, 0), payload.bounce - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, reflectionRay, reflectionPayload);
			totalLight += R.w * material.Specular * reflectionPayload.color; /// Mirror or Fresnel reflection
		}

		if (T.w > 0) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.001;
			refractionRay.Direction = T.xyz;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RayPayload refractionPayload = { float3(0, 0, 0), payload.bounce - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractionRay, refractionPayload);
			totalLight += T.w * material.Specular * refractionPayload.color;
		}
	}
	payload.color = totalLight;
}


[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	//payload.color = float4(WorldRayDirection(), 1);
}