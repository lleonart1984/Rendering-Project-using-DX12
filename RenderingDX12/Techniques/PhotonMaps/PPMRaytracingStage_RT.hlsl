/// A common header for libraries will use a GBuffer for deferred gathering
// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

#define VERTICES_REG				t1
#define MATERIALS_REG				t2
#define GBUFFER_POSITIONS_REG		t3
#define GBUFFER_NORMALS_REG			t4
#define GBUFFER_COORDINATES_REG		t5
#define GBUFFER_MATERIALS_REG		t6
#define TEXTURES_REG				t7

#define TEXTURES_SAMPLER_REG		s0

#define VIEWTOWORLD_CB_REG			b0
#define LIGHTING_CB_REG				b1
#define OBJECT_CB_REG				b2

#include "../CommonRT/CommonDeferred.h"
#include "../CommonGI/ScatteringTools.h"
#include "../CommonGI/Parameters.h"

struct RayHitInfo {
	Vertex Surfel;
	float2 Grad;
	float3 D; // Direction
	float3 I; // Importance
	int MI; // Material index
};

struct RayHitAccum {
	float N;
	float3 Accum;
};

RWStructuredBuffer<RayHitInfo> RTHits		: register (u0);
RWTexture2D<int> ScreenHead					: register (u1);
RWStructuredBuffer<int> ScreenNext			: register (u2);
RWStructuredBuffer<int> Malloc				: register (u3);
RWStructuredBuffer<RayHitAccum> PM			: register (u4);
RWStructuredBuffer<float> Radii				: register (u5);

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

void RaytracingScattering(float3 V, Vertex surfel, int materialIndex, Material material, float2 grad, float3 importance, int bounces)
{
	uint2 pixelEntry = DispatchRaysIndex();

	int hIndex;
	InterlockedAdd(Malloc[0], 1, hIndex); // Alloc a new RT hit info

	// Initialize ray hit info entry
	RayHitInfo info = (RayHitInfo)0;
	info.Surfel = surfel;
	info.Grad = grad;
	info.D = V;
	info.I = importance;
	info.MI = materialIndex;
	RTHits[hIndex] = info;

	// Clear photon map accumulator entry
	PM[hIndex] = (RayHitAccum)0;
	
	// Initialize radius as PHOTON_RADIUS
	Radii[hIndex] = PHOTON_RADIUS;

	// Add to screen per-pixel linked-list
	InterlockedExchange(ScreenHead[pixelEntry], hIndex, ScreenNext[hIndex]);


	// Recursive calls
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;

	if (bounces > 0 && any(material.Specular))
	{
		ComputeImpulses(V, surfel, material,
			NdotV,
			invertNormal,
			fN,
			R,
			T);

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
			EncodeRTPayloadAccumulation(reflectionPayload, importance * R.w * material.Specular);
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 1, 0, 1, 0, reflectionRay, reflectionPayload);
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
			EncodeRTPayloadAccumulation(refractionPayload, importance * T.w * material.Specular);
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 1, 0, 1, 0, refractionRay, refractionPayload);
		}
	}
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
	int materialIndex;
	float3 V;
	//float2 coord = float2(raysIndex.x + random(), raysIndex.y + random()) / raysDimensions;
	float2 coord = float2(raysIndex.x + 0.5, raysIndex.y + 0.5) / raysDimensions;

	if (!GetPrimaryIntersection(raysIndex, coord, V, surfel, material, materialIndex))
		// no primary ray hit
		return;

	float4 C = Coordinates[raysIndex];

	RaytracingScattering(V, surfel, materialIndex, material, C.zw, float3(1,1,1), RAY_TRACING_MAX_BOUNCES);
}

[shader("closesthit")]
void RTScattering(inout RTPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	int materialIndex;
	GetHitInfo(attr, surfel, material, materialIndex, 0, 0);

	float3 V = -normalize(WorldRayDirection());

	float4 C = Coordinates[(uint2)DispatchRaysIndex()];

	RaytracingScattering(V, surfel, materialIndex, material, C.zw, DecodeRTPayloadAccumulation(payload), DecodeRTPayloadBounces(payload));
}
