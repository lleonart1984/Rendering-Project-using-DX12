// Grab depth complexity

struct PSInput
{
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

//RasterizerOrderedTexture2D<int> depthComplexity : register(u0);
RWTexture2D<int> depthComplexity : register(u1);

float4 main(PSInput input) : SV_TARGET
{
	//depthComplexity[input.Proj.xy] = 5 + (int)input.Proj.x;
	InterlockedAdd(depthComplexity[input.Proj.xy], 1);
	return float4(input.Proj.xy/1000,1,1);
}