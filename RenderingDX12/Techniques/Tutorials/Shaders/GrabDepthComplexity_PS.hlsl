// Grab depth complexity

struct PSInput
{
	float4 Proj : SV_POSITION;
	float3 P : POSITION;
	float3 N : NORMAL;
	float2 C : TEXCOORD;
};

//RasterizerOrderedTexture2D<int> depthComplexity : register(u0);
RWTexture2D<int> depthComplexity : register(u0);

void main(PSInput input) 
{
	//depthComplexity[input.Proj.xy] ++;
	InterlockedAdd(depthComplexity[input.Proj.xy], 1);
}