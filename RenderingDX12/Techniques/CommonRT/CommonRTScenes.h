/// Common header to any library that will use the scene retained as follow.
/// A buffer with all vertices at VERTICES_REG
/// A buffer with all materials at MATERIALS_REG
/// A texture array with all textures available at TEXTURES_REG
/// A local constant buffer referring for each geometry to start triangle and material index at OBJECT_CB_REG

/// Provides functions to get intersection information
/// Augment material with texture mapping
/// Augment material and surfel with texture mapping (including bump mapping)

#include "../CommonGI/Definitions.h"

StructuredBuffer<Vertex> vertices		: register(VERTICES_REG);
StructuredBuffer<Material> materials	: register(MATERIALS_REG);
// Textures
Texture2D<float4> Textures[500]			: register(TEXTURES_REG);

// Used for texture mapping
SamplerState gSmp : register(TEXTURES_SAMPLER_REG);


cbuffer Lighting : register(LIGHTING_CB_REG) {
	float3 LightPosition;
	float3 LightIntensity;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups (fresnel and lambert)
ConstantBuffer<ObjInfo> objectInfo		: register(OBJECT_CB_REG);

struct ZipColor {
	half intensity;
	unorm half3 spectrum;
};

float3 Float3ToZipColor(ZipColor c) {
	return c.intensity * c.spectrum;
}

ZipColor ZipColorToFloat3(float3 f) {
	float i = max(f.x, max(f.y, f.z));
	ZipColor z = (ZipColor)0;
	z.intensity = i;
	z.spectrum = i == 0 ? 0 : f / i;
	return z;
}

struct ZipNormal {
	half2 uv;
};

float3 ZipNormalToFloat3(ZipNormal normal, float3 faceDir) {
	float3 n = float3(normal.uv, sqrt(1 - dot(normal.uv, normal.uv)));
	return n * sign(dot(n, faceDir));
}

ZipNormal Float3ToZipNormal(float3 v) {
	ZipNormal n = (ZipNormal)0;
	n.uv = v.xy;
	return n;
}

// Given a surfel will modify the normal with texture maps, using
// Bump mapping and masking textures.
// Material info is updated as well.
void AugmentHitInfoWithTextureMapping(inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0) : 1;

	float3x3 TangentToWorld = { surfel.T, surfel.B, surfel.N };
	// Change normal according to bump map
	surfel.N = normalize(mul(BumpTex * 2 - 1, TangentToWorld));

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

// Given a surfel will modify the material using texture mapping.
void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0).xyz : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

void GetHitInfo(in BuiltInTriangleIntersectionAttributes attr, out Vertex surfel, out Material material)
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
