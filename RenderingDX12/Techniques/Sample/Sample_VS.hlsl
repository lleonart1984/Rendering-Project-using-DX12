struct PSInput
{
	float4 projected : SV_POSITION;
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD;
};

cbuffer Globals : register(b0)
{
	row_major matrix projection;
	row_major matrix view;
};

cbuffer Locals : register(b1)
{
	row_major matrix world;
};

PSInput VSMain(float3 position : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL, float3 tangent : TANGENT, float3 binormal : BINORMAL)
{
	PSInput result;

	float4x4 worldview = mul(world, view);

	result.position = mul(float4(position, 1), worldview).xyz;
	result.normal = normalize(mul(float4(normal, 0), worldview).xyz);
	result.tangent = normalize(mul(float4(tangent, 0), worldview).xyz);
	result.binormal = normalize(mul(float4(binormal, 0), worldview).xyz);
	result.projected = mul(float4(result.position, 1), projection);
	result.uv = uv; 

	return result;
}
