#include "../CommonGI/Parameters.h"

Texture3D<float>  DensityCloud : register(t0);

sampler VolumeSampler : register(s0);

RWTexture3D<float> Accumulated : register(u0);

cbuffer Transform : register(b0) {
	row_major matrix FromAccToCloud;
};

float SampleDensity(float3 coord) {
	return DensityCloud.SampleGrad(VolumeSampler, coord, 0, 0);
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int3 DenDimensions;
	DensityCloud.GetDimensions(DenDimensions.x, DenDimensions.y, DenDimensions.z);

	int3 AccDimensions;
	Accumulated.GetDimensions(AccDimensions.x, AccDimensions.y, AccDimensions.z);

	int maxD = max(DenDimensions.x, max(DenDimensions.y, DenDimensions.z));

	float scaleFactor = 0.5 * length(mul(float3(1, 1, 1), FromAccToCloud)) / AccDimensions.x; // improve with matrix indices
	//float scaleFactor = pow(0.5 / AccDimensions.x, 1);// / AccDimensions.x; // improve with matrix indices

	float acc = 0;
	for (int z = 0; z < AccDimensions.z; z++) {
		int3 cell = int3(DTid.xy, z);
		acc += SampleDensity(mul(float4((cell + 0.5) / AccDimensions, 1), FromAccToCloud).xyz) * scaleFactor;
		Accumulated[cell] = acc;
	}
}