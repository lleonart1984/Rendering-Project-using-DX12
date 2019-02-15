struct VS_In {
	float3 P : POSITION;
	float3 C : COLOR;
};

struct PS_In {
	float4 P : SV_POSITION;
	float3 C : COLOR;
};

PS_In main(VS_In input)
{
	PS_In Out = (PS_In)0;

	Out.P = float4(input.P, 1);
	Out.C = input.C;

	return Out;
}