
cbuffer LightInfo : register(b0)
{
	row_major matrix ViewToLightProj;
	row_major matrix ViewToLightView;
};

SamplerState Sampler : register(s0);

Texture2D<float3> Positions : register(t0);
Texture2D<float3> PositionsFromLights : register(t1);
Texture2D<float3> DirectLight : register(t2);
Texture2D<float3> IndirectLight : register(t3);

float4 main(float4 proj : SV_POSITION) : SV_TARGET
{
	float3 P = Positions[proj.xy];
	float4 PInLightViewSpace = mul(float4(P, 1), ViewToLightView);
	float4 PInLightProjSpace = mul(float4(P, 1), ViewToLightProj);

	float2 CToSample = 0.5 + 0.5*PInLightProjSpace.xy / PInLightProjSpace.w;
	CToSample.y = 1 - CToSample.y;

	float3 PSampledFromLight = PositionsFromLights.SampleGrad(Sampler, CToSample, 0, 0);

	float visibility = 1;// (PInLightViewSpace.z - PSampledFromLight.z) < 0.01;
	
	//return float4(DirectLight[proj.xy] + IndirectLight[proj.xy], 1);

	return float4( DirectLight[proj.xy] * visibility + IndirectLight[proj.xy], 1);
}