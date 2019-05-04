#include "../CommonGI/Definitions.h"
#include "../Randoms/RandomHLSL.h"

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

#include "../CommonGI/ScatteringHLSL.h"

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
	random();
	// Trace the ray.
	// Set the ray's extents.
	RayDesc ray;
	ray.Origin = LightPosition + float3(random() - 0.5, -0.5, random() - 0.5); // 1 unit quad light
	ray.Direction = normalize(ray.Origin - LightPosition);
	ray.TMin = 0.001;
	ray.TMax = 10000.0;
	// photons travel with a piece of intensity. This could produce numerical problems and it is better to add entire intensity
	// and then divide by the number of photons.
	RayPayload payload = { LightIntensity * 1000000 / (-ray.Direction.y * pi * raysDimensions.x * raysDimensions.y), 2 };
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

[shader("closesthit")]
void PhotonScattering(inout RayPayload payload, in MyAttributes attr)
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
	float3 V = -normalize(WorldRayDirection());

	float NdotV = dot(V, surfel.N);
	bool exiting = NdotV < 0;
	float3 fN = exiting ? -surfel.N : surfel.N;

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
		//float3 ratio, direction;
		//float pdf;

		//RandomScatter(V, surfel, exiting, material, direction, ratio, pdf);

		//if (any(ratio))
		//{
		//	RayDesc newPhotonRay;
		//	newPhotonRay.Direction = direction;
		//	newPhotonRay.Origin = surfel.P + sign(dot(newPhotonRay.Direction, surfel.N))*0.0001*surfel.N; // avoid self shadowing
		//	newPhotonRay.TMin = 0.001;
		//	newPhotonRay.TMax = 100000;

		//	RayPayload newPhotonPayload = { payload.color * ratio / pdf, payload.bounce - 1 };

		//	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, newPhotonRay, newPhotonPayload); // Will be used with Photon scattering function
		//}
	}
}

// Perform photon gathering
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {

	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : 1;

	float3x3 worldToTangent = { surfel.T, surfel.B, surfel.N };

	float3 normal = normalize(mul(BumpTex * 2 - 1, worldToTangent));

	float radius = 0.01;// min(CellSize.x, min(CellSize.y, CellSize.z)) / 2;

	int3 begCell = FromPositionToCell(surfel.P - radius);
	int3 endCell = FromPositionToCell(surfel.P + radius);

	float3 totalLighting = material.Emissive;

	//radius *= 0.6;

	//[unroll(2)]
	for (int dz = begCell.z; dz <= endCell.z; dz++)
	//	[unroll(2)]
	for (int dy = begCell.y; dy <= endCell.y; dy++)
	//	[unroll(2)]
	for (int dx = begCell.x; dx <= endCell.x; dx++)
	{
		int cellIndexToQuery = FromCellToCellIndex(int3(dx, dy, dz));

		if (cellIndexToQuery != -1) // valid coordinates
		{
			int currentPhotonPtr = HeadBuffer[cellIndexToQuery];

			while (currentPhotonPtr != -1) {

				Photon p = Photons[currentPhotonPtr];

				// Aggregate current Photon contribution if inside radius
				if (length(p.Position - surfel.P) < radius)
				{
					float3 H = normalize(V - p.Direction);

					float3 BRDF = material.Diffuse * DiffTex + max(SpecularTex, material.Specular) * pow(saturate(dot(normal, H)), material.SpecularSharpness);

					totalLighting += p.Intensity * BRDF;
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
	float3 V = -normalize(WorldRayDirection());

	float3 totalLight = material.Emissive +
		material.Roulette.x * ComputeDirectLightInWorldSpace(surfel, material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);

	if (payload.bounce > 0)
	{
		bool entering = dot(V, surfel.N) > 0;
		float reflectionIndex, refractionIndex;
		float3 reflectionDir, refractionDir;
		float eta = entering ? 1 / material.Roulette.w : material.Roulette.w;
		float3 fN = entering ? surfel.N : -surfel.N;
		ComputeFresnel(-V, fN, eta,
			reflectionIndex, refractionIndex, reflectionDir, refractionDir);

		if (reflectionIndex > 0) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + fN * 0.0001;
			reflectionRay.Direction = reflectionDir;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RayPayload reflectionPayload = { float3(0, 0, 0), payload.bounce - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, reflectionRay, reflectionPayload);
			totalLight += (material.Roulette.y + material.Roulette.z * reflectionIndex)*reflectionPayload.color; /// Mirror and fresnel reflection
		}

		if (refractionIndex > 0) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.0001;
			refractionRay.Direction = refractionDir;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RayPayload refractionPayload = { float3(0, 0, 0), payload.bounce - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractionRay, refractionPayload);
			totalLight += material.Roulette.z * refractionIndex * refractionPayload.color;
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

