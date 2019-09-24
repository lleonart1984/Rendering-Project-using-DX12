/// Implementation of an iterative and accumulative path-tracing.
/// Primary rays and shadow cast use GBuffer optimization
/// Path-tracing with next-event estimation.

#include "../CommonGI/Definitions.h"

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

#include "../CommonRT/CommonDeferred.h"
#include "../CommonRT/CommonShadowMaping.h"
#include "../CommonRT/CommonProgressive.h"
#include "../CommonRT/CommonOutput.h"
#include "../CommonGI/ScatteringTools.h"

struct RayPayload
{
	float3 Importance;
	float3 AccRadiance;
	int bounce;
	uint seed;
};

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will Trace next ray to continue with if necessary
void SurfelScattering(uint seed, float3 V, Vertex surfel, Material material, inout RayPayload payload)
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
	float pdf;
	RandomScatterRay(seed, V, fN, R, T, material, ratio, direction, pdf);

	// Update gathered Importance to the viewer
	payload.Importance *= ratio;
	payload.bounce--;
	payload.seed = seed;

	if (payload.bounce > 0)
	{
		// Trace new ray
		RayDesc newRay;
		newRay.Origin = surfel.P + sign(dot(direction, fN))*0.001*fN;
		newRay.Direction = direction;
		newRay.TMin = 0.001;
		newRay.TMax = 10.0;
		TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
	}
}

float3 ComputePath(uint seed, float3 V, Vertex surfel, Material material, int bounces)
{
	RayPayload payload = (RayPayload)0;
	payload.Importance = 1;
	payload.bounce = bounces;

	// initial scatter (primary rays)
	SurfelScattering(seed, V, surfel, material, payload);
	
	return payload.AccRadiance;
}


[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
}

[shader("raygeneration")]
void PTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	uint seed = StartRandomSeedForRay(raysDimensions, PATH_TRACING_MAX_BOUNCES, raysIndex, 0, PassCount);

	Vertex surfel;
	Material material;
	float3 V;
	float2 coord = (raysIndex + 0.5) / raysDimensions;

	if (!GetPrimaryIntersection(raysIndex, coord, V, surfel, material))
	{
		AccumulateOutput(raysIndex, 0);
		return;
	}

	float3 color = ComputePath(seed, V, surfel, material, PATH_TRACING_MAX_BOUNCES);

	AccumulateOutput(raysIndex, color);
}

[shader("closesthit")]
void PTScattering(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	// Start static seed here... for some reason, in RTX static fields are not shared
	// between different shader types...
	//StartRandomSeedForRay(DispatchRaysDimensions(), PATH_TRACING_MAX_BOUNCES, DispatchRaysIndex(), payload.bounce, PassCount);

	// This is not a recursive closest hit but it will accumulate in payload
	// all the result of the scattering to this surface
	SurfelScattering(payload.seed, -WorldRayDirection(), surfel, material, payload);
}