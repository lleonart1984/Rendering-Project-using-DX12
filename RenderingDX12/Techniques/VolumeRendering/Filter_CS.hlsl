#include "../CommonGI/Parameters.h"

Texture2D<float4> Radiance			: register(t0); // radiance XYZ, density = W
Texture2D<float4> Positions			: register(t1); // positions XYZ, transmitance pdf = W

RWTexture2D<float3> Accumulation	: register(u0);
RWTexture2D<float3> Output			: register(u1); // Render target

cbuffer PathtracingInfo : register(b0) {
	int PassCount;
};

//float3 NoFilter(uint2 px) {
//	return Radiance[px].xyz;
//}
//
//float3 Gaussian3x3(uint2 px) {
//	float kernel[3][3] = {
//	{1, 2, 1},
//	{2, 4, 2},
//	{1, 2, 1}
//	};
//
//	float3 acc = 0;
//
//	for (int dy = 0; dy < 3; dy++)
//		for (int dx = 0; dx < 3; dx++)
//			acc += kernel[dy][dx] * Radiance[px + int2(dx - 1, dy - 1)].xyz;
//
//	return acc / 16.0;
//}
//
//float3 Gaussian5x5(uint2 px) {
//	float kernel[5][5] = {
//	{1, 4, 6, 4, 1},
//	{4, 16, 24, 16, 4},
//	{6, 24, 36, 24, 6},
//	{4, 16, 24, 16, 4},
//	{1, 4, 6, 4, 1}
//	};
//
//	float3 acc = 0;
//
//	for (int dy = 0; dy < 5; dy++)
//		for (int dx = 0; dx < 5; dx++)
//			acc += kernel[dy][dx] * Radiance[px + 2*int2(dx - 2, dy - 2)].xyz;
//
//	return acc / 256.0;
//}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// No filter
	float3 acc = Radiance[DTid.xy].xyz;
	// Gaussian 3x3
	//float3 acc = Gaussian5x5(DTid.xy);

	//float3 acc = Positions[DTid.xy].w;
	//float3 acc = isinf(Positions[DTid.xy].w) ? float3(1,1,0) : float3(1,0,0);

	Accumulation[DTid.xy] = (Accumulation[DTid.xy] * PassCount + acc) / (PassCount + 1);
	Output[DTid.xy] = Accumulation[DTid.xy];
}