/// A common header for libraries will use a GBuffer for deferred gathering
// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

#define VERTICES_REG				t1
#define MATERIALS_REG				t2
#define GBUFFER_POSITIONS_REG		t3
#define GBUFFER_NORMALS_REG			t4
#define GBUFFER_COORDINATES_REG		t5
#define GBUFFER_MATERIALS_REG		t6
#define LIGHTVIEW_POSITIONS_REG		t7
#define TEXTURES_REG				t8

#define TEXTURES_SAMPLER_REG		s0
#define SHADOW_MAP_SAMPLER_REG		s1

#define OUTPUT_IMAGE_REG			u0

#define VIEWTOWORLD_CB_REG			b0
#define LIGHTING_CB_REG				b1
#define LIGHT_TRANSFORMS_CB_REG		b2
#define ACCUMULATIVE_CB_REG			b3
#define OBJECT_CB_REG				b4

#include "..\CommonRT\CommonDeferred.h"
#include "..\CommonRT\CommonShadowMaping.h"
#include "..\CommonRT\CommonProgressive.h"
#include "..\CommonRT\CommonOutput.h"
#include "..\CommonGI\ScatteringTools.h"

#ifndef RT_CUSTOM_PAYLOAD
struct RTPayload
{
	float3 Accumulation;
	int Bounce;
};
#endif

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

	if (bounces > 0 && any(material.Specular))
	{
		if (R.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + fN * 0.001;
			reflectionRay.Direction = R.xyz;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RTPayload reflectionPayload = { float3(0, 0, 0), bounces - 1 };
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 1, 0, 1, 0, reflectionRay, reflectionPayload);
			total += R.w * material.Specular * reflectionPayload.Accumulation; /// Mirror and fresnel reflection
		}

		if (T.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.001;
			refractionRay.Direction = T.xyz;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RTPayload refractionPayload = { float3(0, 0, 0), bounces - 1 };
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 1, 0, 1, 0, refractionRay, refractionPayload);
			total += T.w * material.Specular * refractionPayload.Accumulation;
		}
	}

	return total;
}

[shader("miss")]
void EnvironmentMap(inout RTPayload payload)
{
	//payload.color = WorldRayDirection();
}

[shader("raygeneration")]
void RTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	Vertex surfel;
	Material material;
	float3 V;
	//float2 coord = float2(raysIndex.x + random(), raysIndex.y + random()) / raysDimensions;
	float2 coord = float2(raysIndex.x + 0.5, raysIndex.y + 0.5) / raysDimensions;
	
	if (!GetPrimaryIntersection(raysIndex, coord, V, surfel, material))
		// no primary ray hit
		return;

	float3 color = RaytracingScattering(V, surfel, material, RAY_TRACING_MAX_BOUNCES);

	AccumulateOutput(raysIndex, color);
}

[shader("closesthit")]
void RTScattering(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	float3 V = -normalize(WorldRayDirection());

	payload.Accumulation = RaytracingScattering(V, surfel, material, payload.Bounce);
}
