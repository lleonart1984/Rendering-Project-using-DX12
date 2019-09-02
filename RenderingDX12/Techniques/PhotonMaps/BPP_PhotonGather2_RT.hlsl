#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#define TEXTURES_REG				t10

#define RAY_CONTRIBUTION_TO_HITGROUPS 1

#include "CommongPhotonGather_RT.hlsl.h"

StructuredBuffer<float> Radii : register(t9);

struct PhotonHitAttributes {
	// Photon Index
	int PhotonIdx;
};

struct PhotonRayPayload
{
	float3 SurfelNormal;
	float3 Accum;
};

#ifdef DEBUG_PHOTONS
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in PhotonHitAttributes attr) {
	
	float3 surfelPosition = WorldRayOrigin() + WorldRayDirection() * 0.5;
	Photon p = Photons[attr.PhotonIdx];
	float radius = Radii[attr.PhotonIdx];

	payload.Accum += float3(1, 1, 1);

	IgnoreHit(); // Continue search to accumulate other photons
}
#else
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in PhotonHitAttributes attr) {
	float3 surfelPosition = WorldRayOrigin() + WorldRayDirection() * 0.5;
	Photon p = Photons[attr.PhotonIdx];
	float radius = Radii[attr.PhotonIdx];
	float area = pi * radius * radius;

	float photonDistance = distance(surfelPosition, p.Position);

	float NdotL = dot(payload.SurfelNormal, -p.Direction);
	float NdotN = dot(payload.SurfelNormal, p.Normal);

	// Aggregate current Photon contribution if inside radius
	if (photonDistance < radius && NdotL > 0.001 && NdotN > 0.001)
	{
		float kernel = 2 * (1 - photonDistance / PHOTON_RADIUS);
		payload.Accum += kernel * p.Intensity * NdotN / area;
	}

	IgnoreHit(); // Continue search to accumulate other photons
}
#endif

[shader("intersection")]
void PhotonGatheringIntersection() {
	int index = PrimitiveIndex();
	PhotonHitAttributes att;
	att.PhotonIdx = index;
	ReportHit(0.5, 0, att);
}
[shader("miss")]
void PhotonGatheringMiss(inout PhotonRayPayload payload)
{
}

float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	PhotonRayPayload photonGatherPayload = {
		/*SurfelNormal*/			surfel.N,
		/*OutDiffuseAccum*/			float3(0,0,0) //,
		///*OutSpecularAccum*/		float3(0,0,0)
	};
	RayDesc ray;
	float3 dir = (float3(0, 0.000002, 0));
	ray.Origin = surfel.P;// -dir * 0.000001;
	ray.Direction = dir;// *0.000002;
	ray.TMin = 0.1;
	ray.TMax = 1;
	// Photon Map trace
	// PhotonMap ADS
	// RAY_FLAG_FORCE_NON_OPAQUE to produce any hit execution
	// ~0 : only photons are considered
	// 0 : Ray contribution to hitgroup index
	// 0 : Multiplier (all geometries AABBs will use the same hit group entry)
	// 1 : Miss index for PhotonGatheringMiss shader
	// ray
	// raypayload
	TraceRay(Scene, RAY_FLAG_FORCE_NON_OPAQUE, 0x80, 0, 0, 1, ray, photonGatherPayload);

#ifdef DEBUG_PHOTONS
	return photonGatherPayload.Accum;
#else
	return photonGatherPayload.Accum * material.Roulette.x * material.Diffuse / pi / 100000;// +material.Specular * photonGatherPayload.OutSpecularAccum;
#endif
}