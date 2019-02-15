struct PS_In {
	float4 P : SV_POSITION;
	float3 C : COLOR;
};

float4 main(PS_In input) : SV_TARGET
{
	return float4(input.C, 1);
}