/// A common header for libraries will use a GBuffer for deferred gathering

#include "PhotonDefinition.h"

#ifndef RAY_CONTRIBUTION_TO_HITGROUPS
#define RAY_CONTRIBUTION_TO_HITGROUPS 0
#endif

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

#ifndef LIGHTVIEW_POSITIONS_REG
#define LIGHTVIEW_POSITIONS_REG		t7
#endif

#ifndef TEXTURES_REG
#define TEXTURES_REG				t8
#endif

#ifndef TEXTURES_SAMPLER_REG
#define TEXTURES_SAMPLER_REG		s0
#endif

#ifndef SHADOW_MAP_SAMPLER_REG
#define SHADOW_MAP_SAMPLER_REG		s1
#endif // !SHADOW_MAP_SAMPLER_REG


#ifndef VIEWTOWORLD_CB_REG
#define VIEWTOWORLD_CB_REG			b0
#endif

#ifndef LIGHTING_CB_REG
#define LIGHTING_CB_REG				b1
#endif

#ifndef ACCUMULATIVE_CB_REG
#define ACCUMULATIVE_CB_REG			b2
#endif

#ifndef LIGHT_TRANSFORMS_CB_REG
#define LIGHT_TRANSFORMS_CB_REG		b3
#endif // !LIGHT_TRANSFORMS_CB_REG


#ifndef OBJECT_CB_REG
#define OBJECT_CB_REG				b4
#endif

#ifndef	OUTPUT_IMAGE_REG
#define OUTPUT_IMAGE_REG			u0
#endif

#ifndef RT_CUSTOM_PAYLOAD
struct RTPayload
{
	float3 Accumulation;
	int Bounce;
};
#endif

#include "..\CommonRT\CommonDeferred.h"
#include "..\CommonRT\CommonShadowMaping.h"
#include "..\CommonRT\CommonProgressive.h"
#include "..\CommonRT\CommonOutput.h"
#include "..\CommonGI\ScatteringTools.h"

// Perform photon gathering
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V);

float3 GetColor(int complexity) {

	if (complexity == 0)
		return 0;// float3(1, 1, 1);

	//return float3(1,1,1);

	float level = log(complexity)/log(4);
	float3 stopPoints[8] = {
		float3(0,0,0.1), // 1
		float3(0,0,1), // 4
		float3(0,1,1), // 16
		float3(0,1,0), // 64
		float3(1,1,0), // 256
		float3(1,0,0), // 1024
		float3(1,0,1), // 4096
		float3(1,1,1) // 16000
	};

	if (level >= 7)
		return stopPoints[7];

	return lerp(stopPoints[(int)level], stopPoints[(int)level + 1], level % 1);
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
#ifndef DEBUG_PHOTONS
	total += ComputeDirectLighting(
		V,
		surfel,
		material,
		LightPosition,
		LightIntensity,
		/*Out*/ NdotV, invertNormal, fN, R, T);
#endif

	float3 gatheredLighting = ComputeDirectLightInWorldSpace(surfel, material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);

	// Get indirect diffuse light compute using photon map
#ifndef DEBUG_PHOTONS
	total += gatheredLighting;
#else
	total += GetColor(gatheredLighting);
#endif

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
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, RAY_CONTRIBUTION_TO_HITGROUPS, 1, 0, reflectionRay, reflectionPayload);
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
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, RAY_CONTRIBUTION_TO_HITGROUPS, 1, 0, refractionRay, refractionPayload);
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

	// V is the viewer here
	float3 V;
	float2 coord = (raysIndex + 0.5) / raysDimensions;

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
