#include "Parameters.h"
#include "ScatteringTools.h"
#include "RayQueue.hlsl.h"

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(t0);
Texture2D<float3> Normals				: register(t1);
Texture2D<float4> Coordinates			: register(t2);
Texture2D<int> MaterialIndices			: register(t3);

StructuredBuffer<Material> materials	: register(t4);
// Textures
Texture2D<float4> Textures[500]			: register(t5);

// Used for texture mapping
SamplerState gSmp : register(s0);

// Global constant buffer with view to world transform matrix
cbuffer ViewToWorldTransform : register(b0) {
	row_major matrix ViewToWorld;
}

float ShadowCast(Vertex surfel) {
	return 1;
}

// Given a surfel will modify the material using texture mapping.
void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material, float ddx, float ddy) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, ddx, ddy) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

bool GetPrimaryIntersection(uint2 screenCoordinates, float2 coordinates, out float3 V, out Vertex surfel, out Material material) {
	bool valid = any(Positions[screenCoordinates]);

	float4 C = Coordinates[screenCoordinates];

	surfel.P = Positions.SampleGrad(gSmp, coordinates, 0, 0);// [screenCoordinates];
	float d = length(surfel.P);

	surfel.N = Normals.SampleGrad(gSmp, coordinates, 0, 0);// [screenCoordinates];
	surfel.C = C.xy;//// .SampleGrad(gSmp, coordinates, 0.0001, 0.0001);// [screenCoordinates];
	surfel.T = float3(1, 0, 0);
	surfel.B = cross(surfel.N, surfel.T);


	V = -surfel.P / d;

	material = materials[MaterialIndices[screenCoordinates]];

	V = mul(float4(V, 0), ViewToWorld).xyz;

	surfel = Transform(surfel, ViewToWorld);

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material, C.z, C.w);

	return valid;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint2 screenPx = DTid.xy;
	uint2 dimensions;
	Positions.GetDimensions(dimensions.x, dimensions.y);
	float2 coordinates = (screenPx + 0.5) / dimensions;

	float3 V;
	Vertex surfel;
	Material material;

	if (!GetPrimaryIntersection(screenPx, coordinates, V, surfel, material))
		return;

	if (!any(material.Specular))
		return;

	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;

	ComputeImpulses(V, surfel, material,
		NdotV,
		invertNormal,
		fN,
		R,
		T);

	if (R.w > 0.01)
		YieldRay(screenPx, surfel.P + fN * 0.001, R.xyz, R.w * material.Specular);
	if (T.w > 0.01)
		YieldRay(screenPx, surfel.P - fN * 0.001, T.xyz, T.w * material.Specular);
}