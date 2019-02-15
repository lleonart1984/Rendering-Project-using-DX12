cbuffer cbGlobals : register(b0)
{
	row_major matrix Projection;
	row_major matrix View;
};

cbuffer cbLocals : register(b1)
{
	row_major matrix World;
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float3 Pos : POSITION;
	float3 Nor : NORMAL;
	float2 Coord : TEXCOORD;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;

	output.Pos = mul(float4(input.Pos, 1), World);
	float4 hN = mul(float4(input.Nor, 0), World);
	hN = mul(hN, View);

	output.Pos = mul(output.Pos, View);
	output.P = output.Pos.xyz;
	output.N = hN.xyz;

	output.Pos = mul(output.Pos, Projection);

	output.C = input.Coord;
	//output.C = input.Pos.xy;
	/*output.Pos.x = input.Pos.x * 0.5;
	output.Pos.y = input.Pos.y * 0.5;
	output.Pos.z = 0.4;
	output.Pos.w = 1;*/


	return output;
}
