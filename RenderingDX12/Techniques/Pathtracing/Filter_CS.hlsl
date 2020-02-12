
#include "../CommonGI/Parameters.h"
#include "../CommonGI/Definitions.h"

Texture2D<float3> Accumulation	: register(t0);
Texture2D<float3> Background	: register(t1);
Texture2D<float3> Positions		: register(t2);
Texture2D<float3> Normals		: register(t3);
Texture2D<float4> Coordinates	: register(t4);
Texture2D<int> MaterialIndices	: register(t5);

StructuredBuffer<Material> Materials : register(t6);

Texture2D<float4> Textures [500] : register(t7);

RWTexture2D<float3> Final		: register(u0);

cbuffer AccumulativeInfo : register(b0) {
	int PassCount;
}

sampler gSmp : register(s0);

// Given a surfel will modify the material using texture mapping.
void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material, float ddx, float ddy) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, ddx, ddy) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

bool GetPrimaryIntersection(uint2 screenCoordinates, float2 coordinates, out Vertex surfel, out Material material) {
	bool valid = any(Positions[screenCoordinates]);

	float4 C = Coordinates[screenCoordinates];

	surfel.P = Positions[screenCoordinates];
	float d = length(surfel.P);

	surfel.N = Normals[screenCoordinates];
	surfel.C = C.xy;//// .SampleGrad(gSmp, coordinates, 0.0001, 0.0001);// [screenCoordinates];
	surfel.T = float3(1, 0, 0);
	surfel.B = cross(surfel.N, surfel.T);

	material = Materials[MaterialIndices[screenCoordinates]];

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material, C.z, C.w);

	return valid;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	Vertex surfel;
	Material material;

	if (!GetPrimaryIntersection(DTid.xy, DTid.xy + 0.5, surfel, material))
		return;

	int window = 4;

	float3 final = 0;
	float kernelInt = 0;
	for (int dy=-window; dy<=window; dy++)
		for (int dx = -window; dx <= window; dx++)
		{
			int2 adjPos = DTid.xy + int2(dx, dy);
			float windowKernel = max(0, window - sqrt(dx*dx + dy*dy));// 2 - (dx / (float)window + dy / (float)window);
			float positionKernel = exp(-5*length(Positions[adjPos] - surfel.P));
			float normalKernel = pow(max(0, dot(Normals[adjPos], surfel.N)), 4);

			float kernel = windowKernel * positionKernel * normalKernel;
			final += Accumulation[adjPos] * kernel;
			kernelInt += kernel;
		}

	Final[DTid.xy] = Background[DTid.xy] + material.Diffuse*(final / kernelInt / (PassCount + 1));
}