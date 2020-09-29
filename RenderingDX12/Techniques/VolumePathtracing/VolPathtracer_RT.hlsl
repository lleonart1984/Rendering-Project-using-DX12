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
#define BACKGROUND_IMG_REG			t8
#define TEXTURES_REG				t9

#define TEXTURES_SAMPLER_REG		s0
#define SHADOW_MAP_SAMPLER_REG		s1

#define OUTPUT_IMAGE_REG			u0
#define ACCUM_IMAGE_REG				u1

#define VIEWTOWORLD_CB_REG			b0
#define LIGHTING_CB_REG				b1
#define LIGHT_TRANSFORMS_CB_REG		b2
#define ACCUMULATIVE_CB_REG			b3
#define OBJECT_CB_REG				b6

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
	int bounces;
	uint4 seed;
};

cbuffer ParticipatingMedia : register(b4) {
	float Extinction;
	float Absorption;
}

cbuffer Transforming : register(b5) {
	row_major matrix FromProjectionToWorld;
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
	float pdf;
	RandomScatterRay(V, surfel.N, R, T, material, ratio, direction, pdf);

	// Update gathered Importance to the viewer
	payload.Importance *= max(0, ratio);// / (1 - russianRoulette);
	// Update scattered ray
	payload.ScatteredRay.Direction = direction;
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN)) * 0.001 * fN;
}

#include "VolumeScattering.h"

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void VolumeScattering(float3 V, float3 P, inout RayPayload payload)
{
	float3 Ld = LightPosition - P;
	float d = length(Ld);
	
	if (d < 0.001 || random() < Absorption)
	{
		payload.Importance = 0; // absorption
		return;
	}

	Ld /= d;
	
	payload.AccRadiance += VolumeShadowCast(P) * payload.Importance *
		EvalPhase(0, -V, Ld) * exp(-d * Extinction) * LightIntensity / (6 * pi * d * d);

	float3 newDir = GeneratePhase(0, -V);

	// Update scattered ray
	payload.ScatteredRay.Direction = newDir;
	payload.ScatteredRay.Position = P;
}

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void SurfelScatteringWithoutAccumulation(float3 V, Vertex surfel, Material material, inout RayPayload payload)
{
	// Adding emissive and direct lighting
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;
	ComputeImpulses(V, surfel, material,
		NdotV,
		invertNormal,
		fN,
		R,
		T);

	float3 ratio;
	float3 direction;
	float pdf;
	RandomScatterRay(V, surfel.N, R, T, material, ratio, direction, pdf);

	// Update gathered Importance to the viewer
	payload.Importance *= max(0, ratio);// / (1 - russianRoulette);
	// Update scattered ray
	payload.ScatteredRay.Direction = direction;
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN)) * 0.001 * fN;
}



float3 ComputePath(bool hit, float3 P, float3 V, Vertex surfel, Material material, int bounces)
{
	RayPayload payload = (RayPayload)0;
	payload.Importance = 1;

	float d = length(surfel.P - P);

	float t = -log(max(0.0000001, 1 - random())) / Extinction;

	// initial scatter (primary rays)
	if (hit && t > d)
		SurfelScatteringWithoutAccumulation(V, surfel, material, payload);
	else
		VolumeScattering(V, P - t * V, payload);

	payload.seed = getRNG();

	while(any(payload.Importance > 0.001))
	{
		RayDesc newRay;
		newRay.Origin = payload.ScatteredRay.Position;
		newRay.Direction = payload.ScatteredRay.Direction;
		newRay.TMin = 0.001;
		newRay.TMax = 10.0;

		TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
	}
	return payload.AccRadiance;
}


[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	// Start seed here...
	setRNG(payload.seed);

	float t = -log(max(0.001, 1 - random())) / Extinction;

	float3 P = payload.ScatteredRay.Position + t * payload.ScatteredRay.Direction;

	float roulette = random();

	if (roulette < 0.5) 
		payload.Importance = 0;
	else // hit media particle
	{
		payload.Importance *= 2;
		VolumeScattering(payload.ScatteredRay.Direction, P, payload);
	}
	payload.seed = getRNG();
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
	float3 P;
	float2 coord = (raysIndex + 0.5) / raysDimensions;

	bool hit = GetPrimaryIntersection(raysIndex, coord, P, V, surfel, material);

	float4 ndcP = float4(2 * (raysIndex.xy + 0.5) / raysDimensions - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	V = -D;

	float3 color = ComputePath(hit, P, V, surfel, material, PATH_TRACING_MAX_BOUNCES);
	AccumulateOutput(raysIndex, color);
}

[shader("closesthit")]
void PTScattering(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material, 0, 0);

	// Start seed here...
	setRNG(payload.seed);

	float d = length(surfel.P - payload.ScatteredRay.Position);

	float t = -log(max(0.00000001, 1 - random())) / Extinction;

	if (t > d || dot(payload.ScatteredRay.Direction, surfel.N) < 0) // hit opaque surface or travel inside volume
	{
		if (payload.bounces > PATH_TRACING_MAX_BOUNCES) {
			payload.Importance = 0;
		}
		else {
			payload.bounces++;
			// This is not a recursive closest hit but it will accumulate in payload
			// all the result of the scattering to this surface
			SurfelScattering(-WorldRayDirection(), surfel, material, payload);
		}
	}
	else // hit media particle
		VolumeScattering(-WorldRayDirection(), payload.ScatteredRay.Position + t * payload.ScatteredRay.Direction, payload);

	payload.seed = getRNG();
}