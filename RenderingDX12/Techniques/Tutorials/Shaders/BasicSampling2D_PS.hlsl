struct PS_In {
	float4 P : SV_POSITION;
	float2 C : TEXCOORD;
};

// Texture2D object used
Texture2D Texture : register(t0);

// Sampler used
sampler Sampler : register(s0);

float4 main(PS_In input) : SV_TARGET
{
	return Texture.Sample(Sampler, input.C);
	//return float4(input.C, 0 ,1);
}