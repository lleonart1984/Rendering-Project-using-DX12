struct VS_In {
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

struct PS_In {
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

cbuffer CameraTransforms : register(b0) {
	row_major matrix Projection;
	row_major matrix View;
};

cbuffer WorldTransforms : register(b1) {
	row_major matrix World;
}

PS_In main(VS_In input)
{
	PS_In Out = (PS_In)0;

	Out.P = mul(mul(float4(input.P, 1), World), View).xyz;
	Out.N = mul(mul(float4(input.N, 0), World), View).xyz;
	Out.C = input.C;
	Out.Proj = mul(float4(Out.P, 1), Projection);

	return Out;
}