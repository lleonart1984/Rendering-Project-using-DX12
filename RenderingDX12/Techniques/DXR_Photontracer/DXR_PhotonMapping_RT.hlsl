#include "../CommonGI/Definitions.h"

// Photon Data
struct Photon {
	float3 Position;
	float3 Direction;
	float3 Intensity;
};

// Top level structure with the scene
RaytracingAccelerationStructure Scene	: register(t0, space0);
StructuredBuffer<Vertex> vertices		: register(t1);
StructuredBuffer<Material> materials	: register(t2);
Texture2D<float4> Textures[200]			: register(t3);

// Raytracing output image
RWTexture2D<float4> RenderTarget : register(u0);

// Photon Map binding objects
RWStructuredBuffer<int> HeadBuffer : register(u1);
RWStructuredBuffer<int> Malloc : register(u2);
RWStructuredBuffer<Photon> Photons : register(u3);
RWStructuredBuffer<int> NextBuffer : register(u4);

SamplerState gSmp : register(s0);

// Global constant buffer with projection to world transform matrix
cbuffer Camera : register(b0) {
	row_major matrix ProjectionToWorld;
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

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b3);

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

	// Initialize random iterator using rays index as seed
	initializeRandom(raysIndex.x + raysIndex.y*raysDimensions.x);
	// Trace the ray.
	// Set the ray's extents.
	RayDesc ray;
	ray.Origin = LightPosition;
	float3 exiting = float3(random() * 2 - 1, -1, random() * 2 - 1);
	float fact = length(exiting);
	ray.Direction = exiting / fact;
	ray.TMin = 0.001;
	ray.TMax = 10000.0;
	// photons travel with a piece of intensity. This could produce numerical problems and it is better to add entire intensity
	// and then divide by the number of photons.
	RayPayload payload = { LightIntensity * 1000000 / (4 * pi * pi * fact * fact * raysDimensions.x * raysDimensions.y), 3 };
	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload); // Will be used with Photon scattering function
}

[shader("raygeneration")]
void RTMainRays()
{
	float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

	float2 screenXY = 2 * lerpValues - 1;
	screenXY.y *= -1;

	float4 rayBeg = mul(float4(screenXY, 0, 1), ProjectionToWorld);
	float4 rayEnd = mul(float4(screenXY, 1, 1), ProjectionToWorld);

	float3 origin = rayBeg.xyz / rayBeg.w;
	float3 rayDir = normalize(rayEnd.xyz / rayEnd.w - origin);

	// Trace the ray.
	// Set the ray's extents.
	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = rayDir;
	ray.TMin = 0.001;
	ray.TMax = 10000.0;
	RayPayload payload = { float3(0, 0, 0), 3 };
	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

	// Write the raytraced color to the output texture.
	RenderTarget[DispatchRaysIndex().xy] = float4(payload.color, 1);
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

	if (material.Roulette.x > 0) // Material has some diffuse component
	{ // Store the photon in the photon map
		int cellIndex = FromPositionToCellIndex(surfel.P); // get the cellindex of the volume grid given the position in space
		if (cellIndex != -1) {
			int photonIndexInBuffer;
			InterlockedAdd(Malloc[0], 1, photonIndexInBuffer);
			InterlockedExchange(HeadBuffer[cellIndex], photonIndexInBuffer, NextBuffer[photonIndexInBuffer]); // Update head and next reference

			Photon p = {
				surfel.P,
				WorldRayDirection(), // photon direction
				payload.color // photon intensity
			};
			Photons[photonIndexInBuffer] = p;
		}
	}

	//if (false)
	if (payload.bounce > 0) // Photon can bounce one more time
	{
		float3 ratio, direction;
		RandomScatterRay(V, surfel, material, ratio, direction);

		if (any(ratio))
		{
			RayDesc newPhotonRay;
			newPhotonRay.Direction = direction;
			newPhotonRay.Origin = surfel.P + sign(dot(direction, surfel.N))*0.001*surfel.N; // avoid self shadowing
			newPhotonRay.TMin = 0.001;
			newPhotonRay.TMax = 100000;

			RayPayload newPhotonPayload = { payload.color * ratio, payload.bounce - 1 };

			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, newPhotonRay, newPhotonPayload); // Will be used with Photon scattering function
		}
	}
}

// Perform photon gathering
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {

	float radius = 0.05;// 4 * max(CellSize.x, max(CellSize.y, CellSize.z));

	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);

	float3 totalLighting = material.Emissive;

	for (int dz = begCell.z; dz <= endCell.z; dz++)
	for (int dy = begCell.y; dy <= endCell.y; dy++)
	for (int dx = begCell.x; dx <= endCell.x; dx++)
	{
		int cellIndexToQuery = FromCellToCellIndex(int3(dx, dy, dz));

		if (cellIndexToQuery != -1) // valid coordinates
		{
			int currentPhotonPtr = HeadBuffer[cellIndexToQuery];

			while (currentPhotonPtr != -1) {

				Photon p = Photons[currentPhotonPtr];

				float NdotL = dot(surfel.N, -p.Direction);
				
				float photonDistance = length(p.Position - surfel.P);

				// Aggregate current Photon contribution if inside radius
				if (photonDistance < radius && NdotL > 0)
				{
					// Lambert Diffuse component (normalized dividing by pi)
					float3 DiffuseRatio = DiffuseBRDF(V, -p.Direction, surfel.N, NdotL, material);
					// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
					float3 SpecularRatio = SpecularBRDF(V, -p.Direction, surfel.N, NdotL, material);

					float kernel = 2 * (1 - photonDistance / radius);

					float3 BRDF =
						material.Roulette.x * DiffuseRatio
						+ material.Roulette.y * SpecularRatio;

					totalLighting += kernel * p.Intensity * BRDF;
				}

				currentPhotonPtr = NextBuffer[currentPhotonPtr];
			}
		}
	}

	return totalLighting / (1000000*pi*radius*radius);
}

[shader("closesthit")]
void RTScattering(inout RayPayload payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	uint triangleIndex = objectInfo.TriangleOffset + PrimitiveIndex();
	uint materialIndex = objectInfo.MaterialIndex;

	Vertex v1 = vertices[triangleIndex * 3 + 0];
	Vertex v2 = vertices[triangleIndex * 3 + 1];
	Vertex v3 = vertices[triangleIndex * 3 + 2];
	Vertex surfel = {
		v1.P * barycentrics.x + v2.P * barycentrics.y + v3.P * barycentrics.z,
		v1.N * barycentrics.x + v2.N * barycentrics.y + v3.N * barycentrics.z,
		v1.C * barycentrics.x + v2.C * barycentrics.y + v3.C * barycentrics.z,
		v1.T * barycentrics.x + v2.T * barycentrics.y + v3.T * barycentrics.z,
		v1.B * barycentrics.x + v2.B * barycentrics.y + v3.B * barycentrics.z
	};

	surfel = Transform(surfel, ObjectToWorld4x3());
	Material material = materials[materialIndex];

	AugmentHitInfoWithTextureMapping(surfel, material);

	float3 V = -normalize(WorldRayDirection());

	float3 totalLight = material.Emissive +
		ComputeDirectLightInWorldSpace(surfel, material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);

	if (payload.bounce > 0)
	{
		float4 R, T;
		float3 fN;
		ComputeImpulses(V, surfel, material, fN, R, T);
		
		if (R.w > 0) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + fN * 0.001;
			reflectionRay.Direction = R.xyz;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RayPayload reflectionPayload = { float3(0, 0, 0), payload.bounce - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, reflectionRay, reflectionPayload);
			totalLight += R.w*material.Specular*reflectionPayload.color; /// Mirror and fresnel reflection
		}

		if (T.w > 0) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.0001;
			refractionRay.Direction = T.xyz;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RayPayload refractionPayload = { float3(0, 0, 0), payload.bounce - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractionRay, refractionPayload);
			totalLight += T.w * material.Specular * refractionPayload.color;
		}
	}
	payload.color = totalLight;
}

[shader("miss")]
void PhotonMiss(inout RayPayload payload) {
	// Do nothing... farewell my photon friend...
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	//payload.color = WorldRayDirection();
}

