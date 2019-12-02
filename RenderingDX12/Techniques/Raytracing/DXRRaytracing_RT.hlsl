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
#define OBJECT_CB_REG				b4

#include "../CommonRT/CommonDeferred.h"
#include "../CommonRT/CommonShadowMaping.h"
#include "../CommonRT/CommonProgressive.h"
#include "../CommonRT/CommonOutput.h"
#include "../CommonGI/ScatteringTools.h"
#include "../CommonGI/Parameters.h"

#ifdef COMPACT_PAYLOAD
struct RTPayload
{
	half Compressed0; // intensity
	int Compressed1; // RGBX : RGB - normalized color, X - bounces
};

float3 DecodeRTPayloadAccumulation(in RTPayload p) {
	float3 rgb = (int3(p.Compressed1 >> 24, p.Compressed1 >> 16, p.Compressed1 >> 8) & 255) / 255.0;
	return rgb * p.Compressed0;
}

int DecodeRTPayloadBounces(in RTPayload p) {
	return p.Compressed1 & 255;
}

void EncodeRTPayloadBounce(inout RTPayload p, int bounces) {
	p.Compressed1 = (p.Compressed1 & ~255) | bounces;
}

void EncodeRTPayloadAccumulation(inout RTPayload p, float3 accumulation) {
	float intensity = max(accumulation.x, max(accumulation.y, accumulation.z));
	int3 norm = 255 * saturate(accumulation / max(0.00001, intensity));
	p.Compressed0 = intensity;
	p.Compressed1 = (p.Compressed1 & 255) | norm.x << 24 | norm.y << 16 | norm.z << 8;
}
#else
struct RTPayload
{
	float3 Intensity; // intensity
	int Bounces; // RGBX : RGB - normalized color, X - bounces
};

float3 DecodeRTPayloadAccumulation(in RTPayload p) {
	return p.Intensity;
}

int DecodeRTPayloadBounces(in RTPayload p) {
	return p.Bounces;
}

void EncodeRTPayloadBounce(inout RTPayload p, int bounces) {
	p.Bounces = bounces;
}

void EncodeRTPayloadAccumulation(inout RTPayload p, float3 accumulation) {
	p.Intensity = accumulation;
}
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
	
	if (bounces == RAY_TRACING_MAX_BOUNCES)
	{
		ComputeImpulses(V, surfel, material,
			NdotV,
			invertNormal,
			fN,
			R,
			T);
	}
	else
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
			RTPayload reflectionPayload = (RTPayload)0;
			EncodeRTPayloadBounce(reflectionPayload, bounces - 1);
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 1, 0, 1, 0, reflectionRay, reflectionPayload);
			total += R.w * material.Specular * DecodeRTPayloadAccumulation(reflectionPayload); /// Mirror and fresnel reflection
		}

		if (T.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.001;
			refractionRay.Direction = T.xyz;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RTPayload refractionPayload = (RTPayload)0;
			EncodeRTPayloadBounce(refractionPayload, bounces - 1);
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 1, 0, 1, 0, refractionRay, refractionPayload);
			total += T.w * material.Specular * DecodeRTPayloadAccumulation(refractionPayload);
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
	GetHitInfo(attr, surfel, material, 0, 0);

	float3 V = -normalize(WorldRayDirection());

	EncodeRTPayloadAccumulation(payload, RaytracingScattering(V, surfel, material, DecodeRTPayloadBounces(payload)));
}
