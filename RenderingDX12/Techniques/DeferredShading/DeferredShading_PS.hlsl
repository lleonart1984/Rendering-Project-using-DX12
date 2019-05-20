struct PSInput
{
	float4 projected : SV_POSITION;
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
	int objectId : OBJECTID;
};

struct Material
{
	float3 Diffuse;
	float RefractionIndex;
	float3 Specular;
	float SpecularSharpness;
	int4 Texture_Index;
	float3 Emissive;
	float4 Roulette;
};

StructuredBuffer<int> MaterialIndexBuffer : register(t0);
StructuredBuffer<Material> Materials : register(t1);
Texture2D<float4> Textures[500] : register(t2);

SamplerState Sampler : register(s0);

void main(PSInput input, out float4 position : SV_TARGET0, out float4 normal : SV_TARGET1, out float4 coordinates : SV_TARGET2, out int materialIndex: SV_TARGET3)
{
	materialIndex = MaterialIndexBuffer[input.objectId];
	Material material = Materials[materialIndex];

	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].Sample(Sampler, input.uv) : float4(1,1,1,1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].Sample(Sampler, input.uv) : 1;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].Sample(Sampler, input.uv) : float3(0.5, 0.5, 1);

	if (MaskTex.x < 0.5 || DiffTex.a < 0.5)
		clip(-1);

	float3x3 tangentToWorld = { input.tangent, input.binormal, input.normal };

	normal = float4(normalize(mul(BumpTex * 2 - 1, tangentToWorld)), 1);
	position = float4(input.position,0);
	coordinates = float4(input.uv, 0, 0);
}