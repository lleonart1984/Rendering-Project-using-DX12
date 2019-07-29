#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#ifndef VERTICES_REG
#define VERTICES_REG				t5
#endif

#ifndef MATERIALS_REG
#define MATERIALS_REG				t6
#endif

#ifndef GBUFFER_POSITIONS_REG
#define GBUFFER_POSITIONS_REG		t7
#endif

#ifndef GBUFFER_NORMALS_REG
#define GBUFFER_NORMALS_REG			t8
#endif

#ifndef GBUFFER_COORDINATES_REG
#define GBUFFER_COORDINATES_REG		t9
#endif

#ifndef GBUFFER_MATERIALS_REG
#define GBUFFER_MATERIALS_REG		t10
#endif

#ifndef LIGHTVIEW_POSITIONS_REG
#define LIGHTVIEW_POSITIONS_REG		t11
#endif

#ifndef TEXTURES_REG
#define TEXTURES_REG				t12
#endif

#include "../CommonDeferredGathering.hlsl.h"

struct AABB {
	float3 minimum;
	float3 maximum;
};

#ifndef PHOTON_AABBS_REG
#define PHOTON_AABBS_REG t1
#endif

#ifndef PHOTON_BUFFER_REG
#define PHOTON_BUFFER_REG t2
#endif

#ifndef GEOMETRY_HASH_TABLE_REG
#define GEOMETRY_HASH_TABLE_REG t3
#endif

#ifndef LINKED_LIST_NEXT_REG
#define LINKED_LIST_NEXT_REG t4
#endif


RaytracingAccelerationStructure PhotonMap	: register(PHOTON_AABBS_REG);
StructuredBuffer<Photon> Photons			: register(PHOTON_BUFFER_REG);
StructuredBuffer<int> HashTable				: register(GEOMETRY_HASH_TABLE_REG);
StructuredBuffer<int> NextBuffer			: register(LINKED_LIST_NEXT_REG);

struct GeometryHitAttributes {
	// Photon Index
	int GeometryIdx;
};

struct PhotonRayPayload
{
	float3 SurfelNormal;
	float3 Albedo;
	float3 Accum;
};

#ifdef DEBUG_PHOTONS
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in GeometryHitAttributes attr) {
	float3 surfelPosition = WorldRayOrigin() + WorldRayDirection() * 0.5;

	int geometryIndex = attr.GeometryIdx;

	int currentNode = HashTable[geometryIndex];

	while (currentNode != null)
	{
		Photon p = Photons[currentNode];

		float3 ab = saturate(abs(surfelPosition - p.Position) / p.Radius);
		float d = max(ab.x, max(ab.y, ab.z));// distance(surfelPosition, p.Position);

		float kernel = pow(d, 180);// 2 - 2 * d / p.Radius;
		if (any(p.Intensity))
			payload.OutDiffuseAccum += kernel * 0.01 * float3(0, 0, 1);
		else
			payload.OutDiffuseAccum += kernel * 0.05 * float3(1, 0, 0);

		currentNode = NextBuffer[currentNode];
	}

	IgnoreHit(); // Continue search to accumulate other photons
}
#else
[shader("anyhit")]
void PhotonGatheringAnyHit(inout PhotonRayPayload payload, in GeometryHitAttributes attr) {
	float3 surfelPosition = WorldRayOrigin() + WorldRayDirection() * 0.5;

	int geometryIndex = attr.GeometryIdx;

	int currentNode = HashTable[geometryIndex];

	while (currentNode != -1)
	{
		Photon p = Photons[currentNode];
		float3 V = WorldRayDirection();
		float3 H = normalize(V - p.Direction);

#ifdef PHOTON_WITH_RADIUS
		float radius = p.Radius;
#else
		float radius = PHOTON_RADIUS;
#endif

		float area = pi * radius * radius;

		float photonDistance = distance(surfelPosition, p.Position);

		float NdotL = dot(payload.SurfelNormal, -p.Direction);
		float NdotN = dot(payload.SurfelNormal, p.Normal);

		// Aggregate current Photon contribution if inside radius
		if (photonDistance < radius && NdotL > 0.001 && NdotN > 0.001)
		{
			float kernel = 2 * (1 - photonDistance / radius);

			float3 BRDF =
				p.Intensity * NdotN * payload.Albedo / area / 100000;// *DiffuseRatio;
				//+ material.Roulette.y * SpecularRatio;

			payload.Accum += kernel * p.Intensity * BRDF;
		}

		currentNode = NextBuffer[currentNode];
	}

	IgnoreHit(); // Continue search to accumulate other photon lists
}
#endif

[shader("intersection")]
void PhotonGatheringIntersection() {
	int index = PrimitiveIndex();
	GeometryHitAttributes att;
	att.GeometryIdx = index;
	ReportHit(0.001, 0, att);
}

float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	PhotonRayPayload photonGatherPayload = {
		/*SurfelNormal*/			surfel.N,
		/*Albedo*/					material.Roulette.x * material.Diffuse / pi,
		/*OutDiffuseAccum*/			float3(0,0,0) //,
		///*OutSpecularAccum*/		float3(0,0,0)
	};
	RayDesc ray;
	float3 dir = normalize(float3(1, 1, 1));
	ray.Origin = surfel.P - dir * 0.0001;
	ray.Direction = dir * 0.0002;
	ray.TMin = 0.00001;
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
	TraceRay(PhotonMap, RAY_FLAG_FORCE_NON_OPAQUE, ~0, 0, 0, 1, ray, photonGatherPayload);

	return photonGatherPayload.Accum;// +material.Specular * photonGatherPayload.OutSpecularAccum;
}