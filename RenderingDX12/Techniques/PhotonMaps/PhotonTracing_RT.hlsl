/// A common header for libraries will use a GBuffer for deferred tracing of photons
/// Requires:
/// Everything related to CommonDeferred header.
/// Additionally a RW structured buffer at OUTPUT_PHOTONS_REG for output photons info

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

#ifndef VERTICES_REG
#define VERTICES_REG				t1
#endif

#ifndef MATERIALS_REG
#define MATERIALS_REG				t2
#endif

#ifndef GBUFFER_POSITIONS_REG
#define GBUFFER_POSITIONS_REG		t3
#endif

#ifndef GBUFFER_NORMALS_REG
#define GBUFFER_NORMALS_REG			t4
#endif

#ifndef GBUFFER_COORDINATES_REG
#define GBUFFER_COORDINATES_REG		t5
#endif

#ifndef GBUFFER_MATERIALS_REG
#define GBUFFER_MATERIALS_REG		t6
#endif

#ifndef TEXTURES_REG
#define TEXTURES_REG				t7
#endif

#ifndef TEXTURES_SAMPLER_REG
#define TEXTURES_SAMPLER_REG		s0
#endif

#ifndef VIEWTOWORLD_CB_REG
#define VIEWTOWORLD_CB_REG			b0
#endif

#ifndef LIGHTING_CB_REG
#define LIGHTING_CB_REG				b1
#endif

#ifndef ACCUMULATIVE_CB_REG
#define ACCUMULATIVE_CB_REG			b2
#endif

#ifndef OBJECT_CB_REG
#define OBJECT_CB_REG				b3
#endif

#ifndef PT_CUSTOM_PAYLOAD
struct PTPayload
{
	float3 PhotonIntensity;
	int PhotonBounce;
	uint seed;
};
#endif

RWStructuredBuffer<Photon> Photons		: register(u0);

cbuffer AccumulativeInfo : register(ACCUMULATIVE_CB_REG) {
	int PassCount;
}

#include "..\CommonRT\CommonDeferred.h"
#include "..\CommonGI\ScatteringTools.h"

// Photon trace stage
// Photon rays has the origin in light source and select a random direction (point source)
[shader("raygeneration")]
void PTMainRays() {
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	
	if (PHOTON_TRACE_MAX_BOUNCES == 0) // no photon trace
		return;

	uint seed = StartRandomSeedForRay(raysDimensions, PHOTON_TRACE_MAX_BOUNCES, raysIndex, PHOTON_TRACE_MAX_BOUNCES, PassCount);

	Vertex surfel;
	Material material;
	// L is the viewer here
	float3 L;
	//float2 coord = float2((raysIndex.x + random()) / raysDimensions.x, (raysIndex.y + random()) / raysDimensions.y);
	float2 coord = float2((raysIndex.x + 0.5) / raysDimensions.x, (raysIndex.y + 0.5) / raysDimensions.y);
	float fact = length(float3(coord, 1));

	int photonIndex = raysIndex.x + raysIndex.y * raysDimensions.x;

	Photons[photonIndex].Intensity = 0;

	if (!GetPrimaryIntersection((int2)(coord * SHADOWMAP_DIMENSION) + 0.5, coord, L, surfel, material))
		// no photon hit
		return;

	PTPayload payload = (PTPayload)0;
	payload.PhotonIntensity = // Photon Intensity
		LightIntensity * 100000
		/
		(3 * pi * pi * fact * fact * raysDimensions.x * raysDimensions.y);
	payload.PhotonBounce = // Photon Bounces Left
		PHOTON_TRACE_MAX_BOUNCES - 1;
	payload.seed = seed;

	float3 direction, ratio;
	float pdf;
	RandomScatterRay(payload.seed, L, surfel, material, ratio, direction, pdf);

	if (any(ratio))
	{
		RayDesc ray;
		ray.Origin = surfel.P + sign(dot(direction, surfel.N))*surfel.N*0.001;
		ray.Direction = direction;
		ray.TMin = 0.001;
		ray.TMax = 10;

		payload.PhotonIntensity *= ratio;
		// Trace the ray.
		// Set the ray's extents.
		if (any(payload.PhotonIntensity))
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, ray, payload); // Will be used with Photon scattering function
	}
}

[shader("miss")]
void PhotonMiss(inout PTPayload payload) {
	// Do nothing... farewell my photon friend...
}

[shader("closesthit")]
void PhotonScattering(inout PTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, PHOTON_TRACE_MAX_BOUNCES, raysIndex, payload.PhotonBounce, PassCount);

	int rayId = raysIndex.x + raysIndex.y * raysDimensions.x;

	float3 V = -WorldRayDirection();

	float NdotV = dot(V, surfel.N);

	float russianRoulette = random(payload.seed);
	float stopPdf;

	if (NdotV > 0.001 && material.Roulette.x > 0) { // store photon assuming this is the last bounce
		Photon p = (Photon)0;
		p.Intensity = payload.PhotonIntensity;
		p.Position = surfel.P;
		p.Normal = surfel.N;
		p.Direction = WorldRayDirection();
		Photons[rayId] = p;
		stopPdf = material.Roulette.x * 0.5;
	}
	else
		stopPdf = 0;

	if (russianRoulette < stopPdf) // photon stay here
	{
		Photons[rayId].Intensity *= (1 / stopPdf);
	}
	else
		if (payload.PhotonBounce > 0) // Photon can bounce one more time
		{
			float3 ratio;
			float3 direction;
			float pdf;
			RandomScatterRay(payload.seed, V, surfel, material, ratio, direction, pdf);

			if (any(ratio))
			{
				PTPayload newPhotonPayload = (PTPayload)0;
				newPhotonPayload.PhotonIntensity = payload.PhotonIntensity * ratio / (1 - stopPdf);
				newPhotonPayload.PhotonBounce = payload.PhotonBounce - 1;
				newPhotonPayload.seed = payload.seed;

				RayDesc newPhotonRay;
				newPhotonRay.TMin = 0.001;
				newPhotonRay.TMax = 10.0;
				newPhotonRay.Direction = direction;
				newPhotonRay.Origin = surfel.P + sign(dot(direction, surfel.N))*0.001*surfel.N; // avoid self shadowing

				TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, ~0, 0, 1, 0, newPhotonRay, newPhotonPayload); // Will be used with Photon scattering function
			}
		}
}
