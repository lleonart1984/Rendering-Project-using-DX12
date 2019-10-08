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

#include "../CommonGI/Parameters.h"

#ifdef COMPACT_PAYLOAD
struct PTPayload
{
	half Compressed0; // intensity
	int Compressed1; // RGBX : RGB - normalized color, X - bounces
};

float3 DecodePTPayloadAccumulation(in PTPayload p) {
	float3 rgb = (int3(p.Compressed1 >> 24, p.Compressed1 >> 16, p.Compressed1 >> 8) & 255) / 255.0;
	return rgb * p.Compressed0;
}

int DecodePTPayloadBounces(in PTPayload p) {
	return p.Compressed1 & 255;
}

void EncodePTPayloadBounce(inout PTPayload p, int bounces) {
	p.Compressed1 = (p.Compressed1 & ~255) | bounces;
}

void EncodePTPayloadAccumulation(inout PTPayload p, float3 accumulation) {
	float intensity = max(accumulation.x, max(accumulation.y, accumulation.z));
	int3 norm = 255 * saturate(accumulation / max(0.00001, intensity));
	p.Compressed0 = intensity;
	p.Compressed1 = (p.Compressed1 & 255) | norm.x << 24 | norm.y << 16 | norm.z << 8;
}
#else
struct PTPayload
{
	float3 Accumulation; // intensity
	int Bounce; // RGBX : RGB - normalized color, X - bounces
};

float3 DecodePTPayloadAccumulation(in PTPayload p) {
	return p.Accumulation;
}

int DecodePTPayloadBounces(in PTPayload p) {
	return p.Bounce;
}

void EncodePTPayloadBounce(inout PTPayload p, int bounces) {
	p.Bounce = bounces;
}

void EncodePTPayloadAccumulation(inout PTPayload p, float3 accumulation) {
	p.Accumulation = accumulation;
}
#endif



RWStructuredBuffer<Photon> Photons		: register(u0);
RWStructuredBuffer<float> RadiiFactor	: register(u1);

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

	int photonIndex = raysIndex.x + raysIndex.y * raysDimensions.x;

	Photons[photonIndex].Intensity = 0;
	RadiiFactor[photonIndex] = 1; // radius factor by default

	if (PHOTON_TRACE_MAX_BOUNCES == 0) // no photon trace
		return;

	StartRandomSeedForRay(raysDimensions, PHOTON_TRACE_MAX_BOUNCES, raysIndex, PHOTON_TRACE_MAX_BOUNCES, PassCount);

	Vertex surfel;
	Material material;
	// L is the viewer here
	float3 L;
	float2 coord = float2((raysIndex.x + random()) / raysDimensions.x, (raysIndex.y + random()) / raysDimensions.y);
	//float2 coord = float2((raysIndex.x + 0.5) / raysDimensions.x, (raysIndex.y + 0.5) / raysDimensions.y);
	float fact = length(float3(coord, 1));

	if (!GetPrimaryIntersection((int2)(coord * SHADOWMAP_DIMENSION) + 0.5, coord, L, surfel, material))
		// no photon hit
		return;

	float3 photonIntensity = 
		LightIntensity * 100000
		/
		(3 * pi * pi * fact * fact * raysDimensions.x * raysDimensions.y);

	float3 direction, ratio;
	float pdf;
	RandomScatterRay(L, surfel, material, ratio, direction, pdf);

	if (any(ratio))
	{
		RayDesc ray;
		ray.Origin = surfel.P + sign(dot(direction, surfel.N))*surfel.N*0.001;
		ray.Direction = direction;
		ray.TMin = 0.001;
		ray.TMax = 10;

		PTPayload payload = (PTPayload)0;
		EncodePTPayloadAccumulation(payload,
			// Photon Intensity after first bounce
			photonIntensity * ratio);
		EncodePTPayloadBounce(payload,
			// Photon Bounces Left
			PHOTON_TRACE_MAX_BOUNCES - 1);
		
		// Trace the ray.
		// Set the ray's extents.
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

	float3 payloadAccumulation = DecodePTPayloadAccumulation(payload);
	int payloadBounce = DecodePTPayloadBounces(payload);

	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, PHOTON_TRACE_MAX_BOUNCES, raysIndex, payloadBounce, PassCount);

	int rayId = raysIndex.x + raysIndex.y * raysDimensions.x;

	float3 V = -WorldRayDirection();

	float d = distance(WorldRayOrigin(), surfel.P);

	float NdotV = dot(V, surfel.N);

	float russianRoulette = random();
	float stopPdf;

	if (NdotV > 0.001 && material.Roulette.x > 0) { // store photon assuming this is the last bounce
		Photon p = (Photon)0;
		p.Intensity = payloadAccumulation;
		p.Position = surfel.P;
		p.Normal = surfel.N;
		p.Direction = WorldRayDirection();
		Photons[rayId] = p;
		stopPdf = payloadBounce == 0 ? 1 : 0.5 * material.Roulette.x * (material.Diffuse.x + material.Diffuse.y + material.Diffuse.z) / 3;
	}
	else
		stopPdf = 0;

	if (russianRoulette < stopPdf) // photon stay here
	{
		Photons[rayId].Intensity *= (1 / stopPdf);
		RadiiFactor[rayId] = min(8, RadiiFactor[rayId] * (1 / stopPdf) );
	}
	else
		if (payloadBounce > 0)
		// Photon can bounce one more time
		{
			RadiiFactor[rayId] = min(8, RadiiFactor[rayId] * (1 / (1 - stopPdf)) * (1 + d * 3));

			float3 ratio;
			float3 direction;
			float pdf;
			RandomScatterRay(V, surfel, material, ratio, direction, pdf);

			if (any(ratio))
			{
				EncodePTPayloadAccumulation(payload, payloadAccumulation * ratio / (1 - stopPdf));
				EncodePTPayloadBounce(payload, payloadBounce - 1);

				RayDesc newPhotonRay;
				newPhotonRay.TMin = 0.001;
				newPhotonRay.TMax = 10.0;
				newPhotonRay.Direction = direction;
				newPhotonRay.Origin = surfel.P + sign(dot(direction, surfel.N))*0.001*surfel.N; // avoid self shadowing

				TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, ~0, 0, 1, 0, newPhotonRay, payload); // Will be used with Photon scattering function
			}
		}
}
