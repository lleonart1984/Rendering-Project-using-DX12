// Blinn-Phong shading

struct PSInput
{
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

cbuffer Lighting : register(b0)
{
	float3 LightPosition;
	float3 LightIntensity;
};

cbuffer Materialing : register(b1)
{
	float3 Diffuse;
	float3 Specular;
	float SpecularSharpness;
	int4 Texture_Mask; // x - Diffuse Texture is present
	float3 Emissive;
	float4 Roulette;
};

Texture2D<float3> Diffuse_Texture : register(t0);

SamplerState Sampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	float3 L = LightPosition - input.P;
	float d = length(L);
	L /= d;

	float3 Lin = LightIntensity / max(0.1, d * d);

	float3 DiffTex = Texture_Mask.x > 0 ? Diffuse_Texture.Sample(Sampler, input.C) : 1;

	float3 V = normalize(-input.P);
	float3 H = normalize(V + L);

	float4 coef = lit(dot(L, input.N), dot(H, input.N), SpecularSharpness);

	float3 light = (Diffuse * DiffTex * coef.y + Specular * coef.z)*Lin + Diffuse * DiffTex * coef.x*0.5;
	return float4(light, 1);
}