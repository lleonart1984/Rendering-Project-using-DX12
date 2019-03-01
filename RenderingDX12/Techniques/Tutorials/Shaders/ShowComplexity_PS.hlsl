struct PS_In {
	float4 P : SV_POSITION;
	float3 C : COLOR;
};

Texture2D<int> Complexity : register(t0);

float4 main(PS_In input) : SV_TARGET
{
	return float4(input.C, 1);
}