// Material info
struct Material
{
	float3 Diffuse;
	float3 Specular;
	float SpecularSharpness;
	int4 Texture_Index; // X - diffuse map, Y - Specular map, Z - Bump Map, W - Mask Map
	float3 Emissive; // Emissive component for lights
	float4 Roulette; // X - Diffuse ammount, Y - Mirror scattering, Z - Fresnell scattering, W - Refraction Index (if fresnel)
};

// Vertex Data
struct Vertex
{
	// Position
	float3 P;
	// Normal
	float3 N;
	// Texture coordinates
	float2 C;
	// Tangent Vector
	float3 T;
	// Binormal Vector
	float3 B;
};

// Raytracing hit data collected (raytracing hit are collected for a defered photon gathering)
struct RaytracingHit {
	int TriangleIndex;
	float2 Coordinates;
	float3 V;
	float3 Importance;
};

// Top level structure with the scene
RaytracingAccelerationStructure Scene		: register(t0);
StructuredBuffer<Vertex> vertices			: register(t1);
StructuredBuffer<Material> materials		: register(t2);

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions					: register(t3);
Texture2D<float3> Normals					: register(t4);
Texture2D<float2> Coordinates				: register(t5);
Texture2D<int> MaterialIndices				: register(t6);
// Used for direct light visibility test
Texture2D<float3> LightPositions			: register(t7);

// Textures
Texture2D<float4> Textures[500]				: register(t8);

// Raytracing radiance (missing diffuse and specular interreflections)
RWTexture2D<float3> Output					: register(u0);
// Number of raytracing hits per screen pixel (up to 2^RaytracingDEEP - 1)
RWTexture2D<int> HitCount					: register(u1);
// Buffer with all raytracing hits. For each screen pixel a 2^RaytracingDEEP-1 block is allocated.
RWStructuredBuffer<RaytracingHit> Hits		: register(u2);

// Used for texture mapping
SamplerState gSmp : register(s0);
// Used for shadow map mapping
SamplerState shadowSmp : register(s1);

// Global constant buffer with view to world transform matrix
cbuffer Camera : register(b0) {
	// Light Space (View) to world space transform
	row_major matrix ViewToWorld;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
}

// Matrices to transform from Light space (proj and view) to world space
cbuffer LightTransforms : register(b2) {
	row_major matrix LightProj;
	row_major matrix LightView;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups
ConstantBuffer<ObjInfo> objectInfo : register(b3);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload
{
	float3 Importance;
	float3 AccColor;
	int bounce;
};

void ComputeFresnel(float3 dir, float3 faceNormal, float ratio, out float reflection, out float refraction)
{
	float f = ((1.0 - ratio) * (1.0 - ratio)) / ((1.0 + ratio) * (1.0 + ratio));

	float Ratio = f + (1.0 - f) * pow((1.0 + dot(dir, faceNormal)), 5);

	reflection = min(1, Ratio);
	refraction = max(0, 1 - reflection);
}

Vertex Transform(Vertex surfel, float4x3 transform) {
	Vertex v = {
		mul(float4(surfel.P, 1), transform),
		mul(float4(surfel.N, 0), transform),
		surfel.C,
		mul(float4(surfel.T, 0), transform),
		mul(float4(surfel.B, 0), transform) };
	return v;
}

void AugmentHitInfoWithTextureMapping(bool onlyMaterial, inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0) : 1;

	if (!onlyMaterial) {
		float3x3 TangentToWorld = { surfel.T, surfel.B, surfel.N };
		// Change normal according to bump map
		surfel.N = normalize(mul(BumpTex * 2 - 1, TangentToWorld));
	}

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

float3 RaytracingScattering(float3 V, Vertex surfel, Material material, int bounces)
{
	float3 total = float3(0, 0, 0);

	float3 L = LightPosition - surfel.P;
	float d = length(L);
	L /= d;
	float3 H = normalize(V + L);
	float3 I = LightIntensity / (2 * 3.14159*d*d);
	float NdotL = dot(surfel.N, L);
	float3 diff = max(0, NdotL)*material.Diffuse*I;
	float3 spec = NdotL > 0 ? pow(max(0, dot(H, surfel.N)), material.SpecularSharpness)*material.Specular*I : 0;

	float3 pInLightViewSpace = mul(float4(surfel.P, 1), LightView).xyz;
	float4 pInLightProjSpace = mul(float4(pInLightViewSpace, 1), LightProj);

	float2 cToTest = 0.5 + 0.5 * pInLightProjSpace.xy / pInLightProjSpace.w;
	cToTest.y = 1 - cToTest.y;

	float3 lightSampleP = LightPositions.SampleGrad(shadowSmp, cToTest, 0, 0);

	float visibility = //cToTest.x < 0 || cToTest.y < 0 || cToTest.x > 1 || cToTest.y > 1 ? 0 :
		((pInLightViewSpace.z) - (lightSampleP.z)) < 0.001;

	total += material.Emissive +
		material.Roulette.x * (diff + spec) * visibility;

	if (bounces > 0)
	{
		bool entering = dot(V, surfel.N) > 0;
		float reflectionIndex, refractionIndex;
		float eta = entering ? 1 / material.Roulette.w : material.Roulette.w;
		float3 fN = entering ? surfel.N : -surfel.N;
		ComputeFresnel(-V, fN, eta,
			reflectionIndex, refractionIndex);
		float3 reflectionDir = reflect(-V, fN);
		float3 refractionDir = refract(-V, fN, eta);
		if (!any(refractionDir))
		{
			reflectionIndex = 1;
			refractionIndex = 0; // total internal reflection
		}

		float reflectionFactor = material.Roulette.y + material.Roulette.z * reflectionIndex;
		float refractionFactor = material.Roulette.z * refractionIndex;

		if (reflectionFactor > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc reflectionRay;
			reflectionRay.Origin = surfel.P + fN * 0.0001;
			reflectionRay.Direction = reflectionDir;
			reflectionRay.TMin = 0.001;
			reflectionRay.TMax = 10000.0;
			RayPayload reflectionPayload = { float3(1,1,1), float3(0, 0, 0), bounces - 1 };
			// Raytracing Trace for reflection
			// Scene ADS
			// RAY_FLAG_FORCE_OPAQUE : no anyhit shaders bound
			// ~O : Consider all triangles
			// 1 : Ray contribution to hitgroup index, starting in 1 because first slot is saved for a unique photon gathering hit group.
			// 1 : Multiplier for geometry indices
			// 0 : Miss shader index for Environment map
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, ~0, 1, 1, 0, reflectionRay, reflectionPayload);
			total += reflectionFactor * reflectionPayload.AccColor; /// Mirror and fresnel reflection
		}

		if (refractionIndex > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			RayDesc refractionRay;
			refractionRay.Origin = surfel.P - fN * 0.0001;
			refractionRay.Direction = refractionDir;
			refractionRay.TMin = 0.001;
			refractionRay.TMax = 10000.0;
			RayPayload refractionPayload = { float3(1,1,1), float3(0, 0, 0), bounces - 1 };
			// Raytracing Trace for refraction
			// Scene ADS
			// RAY_FLAG_FORCE_OPAQUE : no anyhit shaders bound
			// ~O : Consider all triangles
			// 1 : Ray contribution to hitgroup index, starting in 1 because first slot is saved for a unique photon gathering hit group.
			// 1 : Multiplier for geometry indices
			// 0 : Miss shader index for Environment map
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, ~0, 1, 1, 0, refractionRay, refractionPayload);
			total += refractionFactor * refractionPayload.AccColor;
		}
	}
	return total;
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	payload.AccColor = WorldRayDirection();
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
		RayPayload payload = { float3(1,1,1), float3(0,0,0), 0 };
		//EnvironmentMap(payload);
		Output[DispatchRaysIndex().xy] = float4(payload.AccColor, 1);
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
	Output[DispatchRaysIndex().xy] = RaytracingScattering(V, surfel, material, 2);
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

	payload.AccColor = RaytracingScattering(V, surfel, material, payload.bounce);
}
