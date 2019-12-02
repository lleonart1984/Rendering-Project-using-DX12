#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#define TEXTURES_REG				t12

#define RAY_CONTRIBUTION_TO_HITGROUPS 1

#include "CommongPhotonGather_RT.hlsl.h"

RaytracingAccelerationStructure PhotonMap : register(t10);
StructuredBuffer<float> Radii : register(t11);

struct PhotonHitAttributes {
	// AABB Index where you can find photons from AABBIdx*BOXED_PHOTONS + 0 to AABBIdx*BOXED_PHOTONS + BOXED_PHOTONS - 1
	int AABBIdx;
};

struct PhotonRayPayload
{
	float3 SurfelNormal;
	float3 Accum;
};

#ifdef DEBUG_PHOTONS
#if DEBUG_STRATEGY == 0
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in PhotonHitAttributes attr) {

	/*float3 surfelPosition = WorldRayOrigin() + WorldRayDirection() * 0.5;
	Photon p = Photons[attr.PhotonIdx];
	float radius = Radii[attr.PhotonIdx];
	float photonDistance = distance(surfelPosition, p.Position);*/

	//if (photonDistance < radius) // uncomment this to see really considered photons
	payload.Accum += float3(1, 1, 1) * BOXED_PHOTONS;

	IgnoreHit(); // Continue search to accumulate other photons
}
#else
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in PhotonHitAttributes attr) {

	float3 surfelPosition = WorldRayOrigin();
	for (int i = 0; i < BOXED_PHOTONS; i++) {
		int photonIdx = attr.AABBIdx * BOXED_PHOTONS + i;
		Photon p = Photons[photonIdx];
		float radius = Radii[photonIdx];

		float photonDistance = distance(surfelPosition, p.Position);

		if (photonDistance < radius) 
		{
			float radiusRatio = min (1, radius / PHOTON_RADIUS);// min(radius, distance(p.Position, surfelPosition)) / PHOTON_RADIUS;

			if (any(p.Intensity))
				payload.Accum += float3(radiusRatio, 1, 1);
		}
	}
	//}

	IgnoreHit(); // Continue search to accumulate other photons
}
#endif
#else
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in PhotonHitAttributes attr) {
	float3 surfelPosition = WorldRayOrigin();// +WorldRayDirection() * 0.5;
	float3 V = WorldRayDirection();

	[loop]
	for (int i = 0; i < BOXED_PHOTONS; i++) {
		int photonIdx = attr.AABBIdx * BOXED_PHOTONS + i;

		Photon p = Photons[photonIdx];
		float radius = max(0.0001, Radii[photonIdx]);
		float area = radius * radius;

		float photonDistance = distance(surfelPosition, p.Position);

		float NdotL = dot(payload.SurfelNormal, -p.Direction);
		float NdotN = dot(payload.SurfelNormal, p.Normal);
		float NdotV = dot(V, p.Normal);

		// Aggregate current Photon contribution if inside radius
		//float kernel = (photonDistance < radius && NdotL > 0.001);
		float kernel = (NdotV > 0.0001)*(NdotL > 0.001) * max(0, 1 - (photonDistance / radius));
		//float kernel = (photonDistance < radius && NdotL > 0.001) * 2 * (1 - pow(photonDistance / radius,2)) / pi;
		payload.Accum += kernel * p.Intensity * max(0, NdotN) / area;
	}

	IgnoreHit(); // Continue search to accumulate other photons
}
#endif

[shader("intersection")]
void PhotonGatheringIntersection() {
	int index = PrimitiveIndex() + InstanceID();
	PhotonHitAttributes att;
	att.AABBIdx = index;
	ReportHit(0.0001, 0, att);
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
	//float3 dir = normalize(float3(1, 1, 1)) * 0.0002;
	float3 dir = normalize(V);// *0.0002;
	ray.Origin = surfel.P;// -dir * 0.5;
	ray.Direction = dir;// *0.000002;
	ray.TMin = 0.00005;
	ray.TMax = 0.00015;
	// Photon Map trace
	// PhotonMap ADS
	// RAY_FLAG_FORCE_NON_OPAQUE to produce any hit execution
	// ~0 : only photons are considered
	// 0 : Ray contribution to hitgroup index
	// 0 : Multiplier (all geometries AABBs will use the same hit group entry)
	// 1 : Miss index for PhotonGatheringMiss shader
	// ray
	// raypayload
	TraceRay(PhotonMap, RAY_FLAG_FORCE_NON_OPAQUE, ~0, 0, 0, 1, ray, photonGatherPayload);

#ifdef DEBUG_PHOTONS
#if DEBUG_STRATEGY == 0
	// Count photons
	return photonGatherPayload.Accum;
#else
	if (photonGatherPayload.Accum.y >= 1)//DESIRED_PHOTONS / 4)
		return pow(4, (1.001 - min(1, photonGatherPayload.Accum.x / photonGatherPayload.Accum.y)) * 7);
	return 1;

	// Accum is farthest distance
	//if (photonGatherPayload.Accum.y < DESIRED_PHOTONS)
	//	return pow(4, 0);
	//return pow(4, (1 - min(1, photonGatherPayload.Accum.x))*7);// pow(4, 7 * (photonGatherPayload.Accum.x / PHOTON_RADIUS)); // to draw radius contraction
#endif
#else
	return photonGatherPayload.Accum * 2 * material.Roulette.x * material.Diffuse / pi / pi / 100000;// +material.Specular * photonGatherPayload.OutSpecularAccum;
#endif
}