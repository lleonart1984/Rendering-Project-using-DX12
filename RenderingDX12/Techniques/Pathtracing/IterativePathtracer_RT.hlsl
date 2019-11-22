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
#define ACCUM_IMAGE_REG				u1

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

struct Ray {
	float3 Position;
	float3 Direction;
};

struct RayPayload
{
	float3 Importance;
	float3 AccRadiance;
	Ray ScatteredRay;
	int bounce;
};

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
	float pdf;
	RandomScatterRay(V, fN, R, T, material, ratio, direction, pdf);

	// Update gathered Importance to the viewer
	payload.Importance *= max(0, ratio);// / (1 - russianRoulette);
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
	for (int bounce = 1; bounce < bounces; bounce++)
	{
		RayDesc newRay;
		newRay.Origin = payload.ScatteredRay.Position;
		newRay.Direction = payload.ScatteredRay.Direction;
		newRay.TMin = 0.001;
		newRay.TMax = 10.0;

		if (any(payload.Importance > 0.001))
		{
			payload.bounce = bounce;
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
		}
	}

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
	StartRandomSeedForRay(raysDimensions, PATH_TRACING_MAX_BOUNCES + 1, raysIndex, 0, PassCount);

	Vertex surfel;
	Material material;
	float3 V;
	float2 coord = (raysIndex + 0.5) / raysDimensions;

	if (!GetPrimaryIntersection(raysIndex, coord, V, surfel, material))
	{
		AccumulateOutput(raysIndex, 0);
		return;
	}

	float3 color = ComputePath(V, surfel, material, PATH_TRACING_MAX_BOUNCES);

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
	StartRandomSeedForRay(DispatchRaysDimensions(), PATH_TRACING_MAX_BOUNCES + 1, DispatchRaysIndex(), payload.bounce, PassCount);

	// This is not a recursive closest hit but it will accumulate in payload
	// all the result of the scattering to this surface
	SurfelScattering(-WorldRayDirection(), surfel, material, payload);
}