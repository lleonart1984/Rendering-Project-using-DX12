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
// Used for direct light visibility test
Texture2D<float3> LightPositions		: register(t7);
// Photon Map binding objects
StructuredBuffer<int> HeadBuffer		: register(t8);
StructuredBuffer<Photon> Photons		: register(t9);
StructuredBuffer<int> NextBuffer		: register(t10);

// Textures
Texture2D<float4> Textures[500]			: register(t11);

// Raytracing output image
RWTexture2D<float3> Output				: register(u0);

// Used for texture mapping
SamplerState gSmp : register(s0);
// Used for shadow map mapping
SamplerState shadowSmp : register(s1);

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

// Matrices to transform from Light space (proj and view) to world space
cbuffer LightTransforms : register(b3) {
	row_major matrix LightProj;
	row_major matrix LightView;
}

cbuffer ProgressiveArgs : register(b4) {
	int CurrentPass;
};

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b5);

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

// Perform photon gathering
float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {

	float3 normal = surfel.N;// normalize(mul(BumpTex * 2 - 1, worldToTangent));

	float radius = 0.02f;// min(CellSize.x, min(CellSize.y, CellSize.z)) * 0.5;

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

						float lambert = dot(-p.Direction, normal);

						// Aggregate current Photon contribution if inside radius
						if (length(p.Position - surfel.P) < radius && lambert > 0)
						{
							float3 H = normalize(V - p.Direction);

							float3 BRDF = material.Diffuse + material.Specular * pow(saturate(dot(normal, H)), material.SpecularSharpness) / lambert;

							totalLighting += p.Intensity * BRDF;
						}

						currentPhotonPtr = NextBuffer[currentPhotonPtr];
					}
				}
			}

	return totalLighting / (pi*radius*radius * 100000);
}

#define LightRadius 0.5

// Gets true if current surfel is lit by the light source
// checking not with DXR but with classic shadow maps represented
// by GBuffer obtained from light
bool ShadowCast(Vertex surfel)
{
	float3 pInLightViewSpace = mul(float4(surfel.P, 1), LightView).xyz;
	float4 pInLightProjSpace = mul(float4(pInLightViewSpace, 1), LightProj);
	float2 cToTest = 0.5 + 0.5 * pInLightProjSpace.xy / pInLightProjSpace.w;
	cToTest.y = 1 - cToTest.y;
	float3 lightSampleP = LightPositions.SampleGrad(shadowSmp, cToTest, 0, 0);
	return pInLightViewSpace.z - lightSampleP.z < 0.001;
}

float3 RaytracingScattering(float3 V, Vertex surfel, Material material, int bounces)
{
	float3 total = float3(0, 0, 0);

	// Adding Emissive
	total += material.Emissive;

	float NdotV = dot(V, surfel.N);
	bool exiting = NdotV < 0;
	float3 fN = exiting ? -surfel.N : surfel.N;

	// Adding direct lighting

	float3 L;
	float NdotL;
	float d;
	float3 I = DirectIncomingRadiance(
		LightIntensity,
		LightPosition,
		surfel, fN,
		/*Out*/ L, NdotL, d);

	// Shadow cast using G-Buffer from Light
	if (ShadowCast(surfel))
	{
		// Adding direct diffuse contribution to outgoing radiance
		total += material.Roulette.x * BlinnBRDF(V, L, NdotL, fN, material) * I;

		// Checking direct mirror or fresnel contribution to outgoing radiance
		float reflectionIndex, refractionIndex;
		float3 reflectionDir;
		float3 refractionDir;
		float eta = exiting ? material.Roulette.w : 1 / material.Roulette.w;
		ComputeFresnel(-V, fN, eta,
			reflectionIndex, refractionIndex, reflectionDir, refractionDir);
		
		if (!any(refractionDir))
		{
			reflectionIndex = 1;
			refractionIndex = 0; // total internal reflection
		}

		// Adding direct impulses contribution
		float reflectionFactor = min(1, material.Roulette.y + material.Roulette.z * reflectionIndex);
		float refractionFactor = material.Roulette.z * refractionIndex;

		if (reflectionFactor > 0) // has some reflection contribution
		{
			if (HitLight(L, reflectionDir, d, LightRadius)) // hit the light
				total += LightIntensity * reflectionFactor / (2 * pi*LightRadius*LightRadius * d * d);
		}

		if (refractionFactor > 0) // has some reflection contribution
		{
			if (HitLight(L, refractionDir, d, LightRadius)) // hit the light
				total += LightIntensity * refractionFactor / (2 * pi*LightRadius*LightRadius* d * d);
		}
	}

	// Get indirect diffuse light compute using photon map
	total += ComputeDirectLightInWorldSpace(surfel, material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);

	if (bounces > 0)
	{
		float reflectionIndex, refractionIndex;
		float3 reflectionDir, refractionDir;
		float eta = exiting ? material.Roulette.w : 1 / material.Roulette.w;
		ComputeFresnel(-V, surfel.N, eta,
			reflectionIndex, refractionIndex,
			reflectionDir, refractionDir);

		float reflectionFactor = material.Roulette.y + material.Roulette.z * reflectionIndex;
		float refractionFactor = material.Roulette.z * refractionIndex;

		if (reflectionFactor > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + surfel.N * 0.0001;
			reflectionRay.Direction = reflectionDir;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RayPayload reflectionPayload = { float3(0, 0, 0), bounces - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, reflectionRay, reflectionPayload);
			total += reflectionFactor * reflectionPayload.color; /// Mirror and fresnel reflection
		}

		if (refractionIndex > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - surfel.N * 0.0001;
			refractionRay.Direction = refractionDir;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RayPayload refractionPayload = { float3(0, 0, 0), bounces - 1 };
			TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractionRay, refractionPayload);
			total += refractionFactor * refractionPayload.color;
		}
	}
	return total;
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	//payload.color = WorldRayDirection();
}

[shader("raygeneration")]
void RTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	float3 P = Positions[raysIndex];
	float3 N = Normals[raysIndex];
	float2 C = Coordinates[raysIndex];
	Material material = materials[MaterialIndices[raysIndex]];

	if (!any(P)) // force miss execution
	{
		RayPayload payload = { float3(0,0,0), 0 };
		//EnvironmentMap(payload);
		Output[DispatchRaysIndex().xy] = float4(payload.color, 1);
		return;
	}
	float3 V = normalize(-P); // In view spce "viewer" is positioned in (0,0,0) 

	// Move to world space
	P = mul(float4(P, 1), ViewToWorld).xyz;
	N = mul(float4(N, 0), ViewToWorld).xyz;
	V = mul(float4(V, 0), ViewToWorld).xyz;

	Vertex surfel = {
		P,
		N,
		C,
		float3(0,0,0),
		float3(0,0,0)
	};

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentHitInfoWithTextureMapping(true, surfel, material);

	// Write the raytraced color to the output texture.
	Output[DispatchRaysIndex().xy] = (Output[DispatchRaysIndex().xy] * CurrentPass + RaytracingScattering(V, surfel, material, 2)) / (CurrentPass + 1);
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

	AugmentHitInfoWithTextureMapping(false, surfel, material);
}

[shader("closesthit")]
void RTScattering(inout RayPayload payload, in MyAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	float3 V = -normalize(WorldRayDirection());

	payload.color = RaytracingScattering(V, surfel, material, payload.bounce);
}
