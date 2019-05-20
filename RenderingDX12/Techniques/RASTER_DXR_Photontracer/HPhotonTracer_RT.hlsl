#include "../CommonGI/Definitions.h"

// Photon Data
struct Photon {
	float3 Position;
	float3 Direction;
	float3 Intensity;
};

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);
StructuredBuffer<Vertex> vertices		: register(t1);
StructuredBuffer<Material> materials	: register(t2);

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(t3);
Texture2D<float3> Normals				: register(t4);
Texture2D<float2> Coordinates			: register(t5);
Texture2D<int> MaterialIndices			: register(t6);

// Textures
Texture2D<float4> Textures[500]			: register(t7);

// Photon Map binding objects
RWStructuredBuffer<int> HeadBuffer		: register(u0);
RWStructuredBuffer<int> Malloc			: register(u1);
RWStructuredBuffer<Photon> Photons		: register(u2);
RWStructuredBuffer<int> NextBuffer		: register(u3);

SamplerState gSmp : register(s0);

// Global constant buffer with view to world transform matrix
cbuffer Camera : register(b0) {
	row_major matrix ViewToWorld;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
}

cbuffer SpaceInformation : register(b2) {
	// Grid space
	float3 MinimumPosition;
	float3 BoxSize;
	float3 CellSize;
	int3 Resolution;
}

cbuffer ProgressivePass : register(b3) {
	int CurrentPass;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b4);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload
{
	float3 color;
	int bounce;
};

#include "../CommonGI/ScatteringTools.h"

int3 FromPositionToCell(float3 P) {
	return (int3)((P - MinimumPosition) / CellSize);
}

int FromCellToCellIndex(int3 cell)
{
	if (any(cell < 0) || any(cell >= Resolution))
		return -1;
	return cell.x + cell.y * Resolution.x + cell.z * Resolution.x * Resolution.y;
}

int3 FromCellIndexToCell(int index) {
	return int3 (index % Resolution.x, index % (Resolution.x * Resolution.y) / Resolution.x, index / (Resolution.x * Resolution.y));
}

int FromPositionToCellIndex(float3 P) {
	return FromCellToCellIndex(FromPositionToCell(P));
}

// Photon trace stage
// Photon rays has the origin in light source and select a random direction (point source)
[shader("raygeneration")]
void PTMainRays() {
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	int rayId = raysIndex.x + raysIndex.y*raysDimensions.x + raysDimensions.x * raysDimensions.y * CurrentPass;
	// Initialize random iterator using rays index as seed
	initializeRandom(rayId);

	RayDesc ray;
	ray.Origin = LightPosition;
	float3 exiting = float3(random() * 2 - 1, -1, random() * 2 - 1);
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

	// Move to world space
	P = mul(float4(P, 1), ViewToWorld).xyz;
	N = mul(float4(N, 0), ViewToWorld).xyz;
	float3 L = normalize(LightPosition - P); // V is really L in this case, since the "viewer" is positioned in light position to trace rays

	RayPayload payload = { LightIntensity * 100000 / (4 * pi * pi * fact * fact * raysDimensions.x * raysDimensions.y), 1 };

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
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload); // Will be used with Photon scattering function
	}
}

[shader("miss")]
void PhotonMiss(inout RayPayload payload) {
	// Do nothing... farewell my photon friend...
}

void GetHitInfo(in MyAttributes attr, out Vertex surfel, out Material material)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	uint triangleIndex = objectInfo.TriangleOffset + PrimitiveIndex();
	uint materialIndex = objectInfo.MaterialIndex;

	Vertex v1 = vertices[triangleIndex * 3 + 0];
	Vertex v2 = vertices[triangleIndex * 3 + 1];
	Vertex v3 = vertices[triangleIndex * 3 + 2];
	Vertex s = {
		v1.P * barycentrics.x + v2.P * barycentrics.y + v3.P * barycentrics.z,
		v1.N * barycentrics.x + v2.N * barycentrics.y + v3.N * barycentrics.z,
		v1.C * barycentrics.x + v2.C * barycentrics.y + v3.C * barycentrics.z,
		v1.T * barycentrics.x + v2.T * barycentrics.y + v3.T * barycentrics.z,
		v1.B * barycentrics.x + v2.B * barycentrics.y + v3.B * barycentrics.z
	};

	surfel = Transform(s, ObjectToWorld4x3());

	material = materials[materialIndex];

	AugmentHitInfoWithTextureMapping(surfel, material);
}

[shader("closesthit")]
void PhotonScattering(inout RayPayload payload, in MyAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	float3 V = -WorldRayDirection();

	float NdotV = dot(V, surfel.N);
	bool exiting = NdotV < 0;
	float3 fN = exiting ? -surfel.N : surfel.N;

	if (material.Roulette.x > 0 && NdotV > 0) // Material has some diffuse component
	{ // Store the photon in the photon map
		int cellIndex = FromPositionToCellIndex(surfel.P); // get the cellindex of the volume grid given the position in space
		if (cellIndex != -1) {
			int photonIndexInBuffer;
			InterlockedAdd(Malloc[0], 1, photonIndexInBuffer);
			InterlockedExchange(HeadBuffer[cellIndex], photonIndexInBuffer, NextBuffer[photonIndexInBuffer]); // Update head and next reference

			Photon p = {
				surfel.P,
				WorldRayDirection(), // photon direction
				payload.color / NdotV // photon intensity
			};
			Photons[photonIndexInBuffer] = p;
		}
	}

	//if (false)
	if (payload.bounce > 0) // Photon can bounce one more time
	{
		float3 ratio;
		float3 direction;
		RandomScatterRay(V, surfel, material, ratio, direction);

		if (any(ratio))
		{
			RayPayload newPhotonPayload = { payload.color * ratio, payload.bounce - 1 };
			RayDesc newPhotonRay;
			newPhotonRay.TMin = 0.001;
			newPhotonRay.TMax = 10000.0;
			newPhotonRay.Direction = direction;
			newPhotonRay.Origin = surfel.P + sign(dot(direction, fN))*0.0001*fN; // avoid self shadowing
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, newPhotonRay, newPhotonPayload); // Will be used with Photon scattering function
		}
	}
}
