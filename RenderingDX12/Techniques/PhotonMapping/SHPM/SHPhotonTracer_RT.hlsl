#include "../../CommonGI/Definitions.h"
#include "../../CommonGI/Parameters.h"

// Photon Data
struct Photon {
	float3 Position;
	float3 Direction;
	float3 Intensity;
};

// Top level structure with the scene
RaytracingAccelerationStructure Scene	: register(t0, space0);

#define VERTICES_REG			t1
#define MATERIALS_REG			t2
#define GBUFFER_POSITIONS_REG	t3
#define GBUFFER_NORMALS_REG		t4
#define GBUFFER_COORDINATES_REG t5
#define GBUFFER_MATERIALS_REG	t6
#define TEXTURES_REG			t7
#define OBJECT_CB_REG			b2
#include "../CommonTracingBindings.hlsli"

// Photon Map binding objects
RWStructuredBuffer<int> HashTable		: register(u0);
RWStructuredBuffer<Photon> Photons		: register(u1);
RWStructuredBuffer<int> NextBuffer		: register(u2);

SamplerState gSmp : register(s0);

// Global constant buffer with view to world transform matrix
cbuffer Camera : register(b0) {
	row_major matrix ViewToWorld;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
}

struct RayPayload
{
	float3 color;
	int bounce;
};

#include "../../CommonGI/ScatteringTools.h"
#include "SHCommon.h"

// Photon trace stage
// Photon rays has the origin in light source and select a random direction (point source)
[shader("raygeneration")]
void PTMainRays() {
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, PHOTON_TRACE_MAX_BOUNCES, raysIndex, PHOTON_TRACE_MAX_BOUNCES, 0);

	RayDesc ray;
	ray.Origin = LightPosition;
	float3 exiting = float3(raysIndex.x*2.0 / raysDimensions.x - 1, -1, raysIndex.y*2.0 / raysDimensions.y - 1);
	float fact = length(exiting);
	ray.Direction = exiting / fact;
	ray.TMin = 0.001;
	ray.TMax = 10000.0;

	float3 P = Positions[raysIndex];
	float3 N = Normals[raysIndex];
	float2 C = Coordinates[raysIndex];
	Material material = materials[MaterialIndices[raysIndex]];
	if (!any(P)) // force miss execution
	{
		return;
	}

	if (PHOTON_TRACE_MAX_BOUNCES == 0) // no photon trace
		return;

	// Move to world space
	P = mul(float4(P, 1), ViewToWorld).xyz;
	N = mul(float4(N, 0), ViewToWorld).xyz;
	float3 L = normalize(LightPosition - P); // V is really L in this case, since the "viewer" is positioned in light position to trace rays

	RayPayload payload = { LightIntensity * 100000 / (4 * pi * pi * fact * fact * raysDimensions.x * raysDimensions.y), PHOTON_TRACE_MAX_BOUNCES - 1 };

	Vertex surfel = {
		P,
		N,
		C,
		float3(0,0,0),
		float3(0,0,0)
	};

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material);

	float3 direction, ratio;
	RandomScatterRay(L, surfel, material, ratio, direction);

	if (any(ratio))
	{
		ray.Origin = surfel.P + sign(dot(direction, surfel.N))*surfel.N*0.001;
		ray.Direction = direction;
		payload.color *= ratio;
		// Trace the ray.
		// Set the ray's extents.
		if (any(payload.color))
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, ray, payload); // Will be used with Photon scattering function
	}
}

[shader("miss")]
void PhotonMiss(inout RayPayload payload) {
	// Do nothing... farewell my photon friend...
}

[shader("closesthit")]
void PhotonScattering(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, PHOTON_TRACE_MAX_BOUNCES, raysIndex, payload.bounce, 0);

	int rayId = raysIndex.x + raysIndex.y * raysDimensions.x ;

	float3 V = -WorldRayDirection();

	float NdotV = dot(V, surfel.N);
	bool exiting = NdotV < 0;
	float3 fN = exiting ? -surfel.N : surfel.N;

	float russianRoulette = random();
	float pdf = material.Roulette.x * 0.5;// *0.002;

	if (russianRoulette < pdf) // photon stay here
	{
		if (NdotV > 0.001)
		{
			Photon p = {
				surfel.P,
				// photon direction
				WorldRayDirection(),
				// photon intensity in a specific area
				payload.color / NdotV * (1 / pdf)
			};

			int cellIndex = FromPositionToCellIndex(surfel.P); // get the cellindex of the volume grid given the position in space
			int photonIndexInBuffer = rayId;
			//InterlockedAdd(Malloc[0], 1, photonIndexInBuffer);
			InterlockedExchange(HashTable[cellIndex], photonIndexInBuffer, NextBuffer[photonIndexInBuffer]); // Update head and next reference
			Photons[photonIndexInBuffer] = p;
		}
	}
	else
		if (payload.bounce > 0) // Photon can bounce one more time
		{
			float3 ratio;
			float3 direction;
			RandomScatterRay(V, surfel, material, ratio, direction);

			if (any(ratio))
			{
				RayPayload newPhotonPayload = { payload.color * ratio / (1 - pdf), payload.bounce - 1 };
				RayDesc newPhotonRay;
				newPhotonRay.TMin = 0.001;
				newPhotonRay.TMax = 10000.0;
				newPhotonRay.Direction = direction;
				newPhotonRay.Origin = surfel.P + sign(dot(direction, fN))*0.0001*fN; // avoid self shadowing
				TraceRay(Scene, RAY_FLAG_NONE, ~0, 0, 1, 0, newPhotonRay, newPhotonPayload); // Will be used with Photon scattering function
			}
		}
}
