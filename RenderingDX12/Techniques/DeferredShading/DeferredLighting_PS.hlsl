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

cbuffer Lighting : register(b0){
	float3 LightPosition;
	float3 LightIntensity;
};

Texture2D<float3> Positions				: register(t0);
Texture2D<float3> Normals				: register(t1);
Texture2D<float2> Coordinates			: register(t2);
Texture2D<int> MaterialIndices			: register(t3);
StructuredBuffer<Material> Materials	: register(t4);
Texture2D<float4> Textures[500]			: register(t5);

SamplerState Sampler : register(s0);

float4 main(float4 proj : SV_POSITION) : SV_TARGET
{
	uint2 coord = proj.xy;

	float3 P = Positions[coord];
	float3 N = Normals[coord];
	float2 C = Coordinates[coord];
	int materialIndex = MaterialIndices[coord];

	Material material = Materials[materialIndex];

	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].Sample(Sampler, C) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].Sample(Sampler, C) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].Sample(Sampler, C) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].Sample(Sampler, C) : 1;

	float3 D = material.Diffuse*DiffTex.xyz;
	float4 S = float4(max(material.Specular, SpecularTex), material.SpecularSharpness);
	float4 R = material.Roulette;

	if (!any(P))
		return float4(1, 0, 1, 1);

	float3 L = LightPosition - P;
	float d = length(L);
	L /= d;
	float3 V = normalize(-P);
	float3 H = normalize(V + L);

	float3 I = LightIntensity / (2*3.14159*d*d);

	float3 diff = max(0, dot(N, L))*D*I;
	float3 spec = pow(max(0, dot(H, N)), S.w)*S.xyz*I;
	return float4((diff + spec)*R.x, 1);
}