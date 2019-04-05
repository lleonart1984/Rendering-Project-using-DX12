
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

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);
StructuredBuffer<Vertex> vertices		: register(t1);
StructuredBuffer<Material> materials	: register(t2);
Texture2D<float4> Textures[200]			: register(t3);

// Raytracing output image
RWTexture2D<float4> RenderTarget : register(u0);

SamplerState gSmp : register(s0);

// Global constant buffer with projection to world transform matrix
cbuffer Camera : register(b0) {
	row_major matrix ProjectionToWorld;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo : register(b1);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload
{
	float3 color;
};

[shader("raygeneration")]
void MainRays()
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
	RayPayload payload = { float3(0, 0, 0) };
	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

	// Write the raytraced color to the output texture.
	RenderTarget[DispatchRaysIndex().xy] = float4( payload.color, 1);
}

float3 ComputeDirectLightInWorldSpace(Vertex surfel, Material material, float3 V) {
	float3 L = normalize(float3(0, 1, 0) - surfel.P);
	float3 Lin = float3(1, 1, 1);

	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0.001, 0.001) : 1;

	float3x3 worldToTangent = { surfel.T, surfel.B, surfel.N };

	float3 normal = normalize(mul(BumpTex * 2 - 1, worldToTangent));

	float3 H = normalize(V + L);

	return material.Diffuse * DiffTex * (saturate(dot(L, normal)) + 0.2) + max(SpecularTex, material.Specular)*pow(max(0, dot(H, normal)), material.SpecularSharpness);
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

[shader("closesthit")]
void LambertScattering(inout RayPayload payload, in MyAttributes attr)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	
	uint triangleIndex = 
		objectInfo.TriangleOffset + 
		PrimitiveIndex();
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

	Material material = materials[materialIndex];
	float3 V = -normalize(WorldRayDirection());

	payload.color = ComputeDirectLightInWorldSpace(Transform(surfel, ObjectToWorld4x3()), material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);
}

[shader("closesthit")]
void FresnelScattering(inout RayPayload payload, in MyAttributes attr)
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

	Material material = materials[materialIndex];
	float3 V = -normalize(WorldRayDirection());

	float3 directLight = ComputeDirectLightInWorldSpace(Transform(surfel, ObjectToWorld4x3()), material, V);// abs(surfel.N);// float3(triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f, triangleIndex % 10000 / 10000.0f);

	// Trace the ray.
	// Set the ray's extents.
	RayDesc reflectionRay;
	reflectionRay.Origin = surfel.P + surfel.N*0.001;
	reflectionRay.Direction = reflect(WorldRayDirection(), surfel.N);
	reflectionRay.TMin = 0.001;
	reflectionRay.TMax = 10000.0;
	RayPayload reflectionPayload = { float3(0, 0, 0) };
	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, reflectionRay, reflectionPayload);

	payload.color = directLight * 0.2 + reflectionPayload.color * 0.8;
}


[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	payload.color = float4(0.1, 0.1, 0.3, 1);
}