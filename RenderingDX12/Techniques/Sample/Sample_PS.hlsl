struct PSInput
{
	float4 projected : SV_POSITION;
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
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
	int4 Texture_Mask;
	float3 Emissive;
	float4 Roulette;
};

Texture2D<float3> Diffuse_Texture : register(t0);
Texture2D<float3> Specular_Texture : register(t1);
Texture2D<float3> Bump_Texture : register(t2);
Texture2D<float3> Mask_Texture : register(t3);
SamplerState Sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
	//return float4(input.normal, 1);
	float3 L = LightPosition - input.position;
	float d = length(L);
	L /= d;

	float3 Lin = LightIntensity / max(0.1, d * d);

	float3 DiffTex = Texture_Mask.x > 0 ? Diffuse_Texture.Sample(Sampler, input.uv) : 1;
	float3 SpecularTex = Texture_Mask.y > 0 ? Specular_Texture.Sample(Sampler, input.uv) : Specular;
	float3 BumpTex = Texture_Mask.z > 0 ? Bump_Texture.Sample(Sampler, input.uv) : float3(0.5, 0.5, 1);
	float3 MaskTex = Texture_Mask.w > 0 ? Mask_Texture.Sample(Sampler, input.uv) : 1;

	if (MaskTex.x < 0.5)
		clip(-1);

	float3x3 worldToTangent = { input.tangent, input.binormal, input.normal };

	float3 normal = normalize(mul(BumpTex * 2 - 1, worldToTangent));
	//return float4(normal, 1);

	float3 V = normalize(-input.position);
	float3 H = normalize(V + L);

	float4 coef = lit(dot(L, normal), dot(H, normal), SpecularSharpness);

	float3 light = (Diffuse * DiffTex * coef.y + SpecularTex * coef.z)*Lin + Diffuse * DiffTex * coef.x*0.5;
	return float4(light, 1);
	//return float4(1,1,0,1);// g_texture.Sample(g_sampler, input.uv);
	//return g_texture.Sample(g_sampler, input.uv);
}