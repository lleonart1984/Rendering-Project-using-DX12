#include "../CommonGI/Parameters.h"
#include "../CommonGI/Definitions.h"
#include "../CommonGI/ScatteringTools.h"

struct RayHitAccum {
	float N;
	float3 Accum;
};

struct RayHitInfo {
	Vertex Surfel;
	float2 Grad;
	float3 D; // Direction
	float3 I; // Importance
	int MI; // Material index
};

StructuredBuffer<RayHitInfo> RTHits		: register(t0);
StructuredBuffer<RayHitAccum> TPM		: register(t1);
StructuredBuffer<float>		Radii		: register(t2);
Texture2D<int>			ScreenHead		: register(t3);
StructuredBuffer<int>	ScreenNext		: register(t4);
StructuredBuffer<Material> materials	: register(t5);
Texture2D<float3> Background			: register(t6);
Texture2D<float4> Textures[500]			: register(t7);

RWTexture2D<float3> Output : register(u0);

SamplerState gSmp : register(s0);

// Given a surfel will modify the material using texture mapping.
void AugmentMaterialWithTextureMapping(Vertex surfel, inout Material material, float ddx, float ddy) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, ddx, ddy) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint2 dim;
	ScreenHead.GetDimensions(dim.x, dim.y);

	if (all(DTid.xy < dim)) // valid pixel
	{
		int currentHit = ScreenHead[DTid.xy];
		
		float3 color = currentHit == -1 ? float3(1,0,1) : 0;

		while (currentHit != -1) {

			RayHitInfo info = RTHits[currentHit];
			RayHitAccum accum = TPM[currentHit];

			float area = Radii[currentHit] * Radii[currentHit] * pi;

			Material material = materials[info.MI];
			AugmentMaterialWithTextureMapping(info.Surfel, material, info.Grad.x, info.Grad.y);

			color += info.I * accum.Accum * 2 * material.Roulette.x * material.Diffuse / area / pi / 100000 / PHOTON_WAVES;

			currentHit = ScreenNext[currentHit];
		}

		Output[DTid.xy] = Background[DTid.xy] + color;
	}
}