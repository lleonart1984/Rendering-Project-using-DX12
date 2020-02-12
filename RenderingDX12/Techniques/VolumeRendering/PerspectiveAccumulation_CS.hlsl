#include "../CommonGI/Parameters.h"

Texture3D<float>  DensityCloud : register(t0);

sampler VolumeSampler : register(s0);

RWTexture3D<float> Accumulated : register(u0);

cbuffer Transforms : register(b0) {
	row_major matrix AccProjMatrixInverse;
	row_major matrix AccViewMatrixInverse;
};

float SampleDensity(float3 coord) {
	return DensityCloud.SampleGrad(VolumeSampler, coord, 0, 0);
}

float3 TransformFromAccToWorld(float3 accPos) {
	float4 p = float4(accPos.xy*2 - 1, accPos.z, 1);
	p = mul(p, AccProjMatrixInverse);
	float3 pView = float3(p.xy * p.w, p.z);

	return mul(float4(pView, 1), AccViewMatrixInverse).xyz;
}

void GetVolumeBox(out float3 minim, out float3 maxim) {
	int3 Dimensions;
	DensityCloud.GetDimensions(Dimensions.x, Dimensions.y, Dimensions.z);

	float maxC = max(Dimensions.x, max(Dimensions.y, Dimensions.z));
	minim = -Dimensions * 0.5 / maxC;
	maxim = -minim;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);
	
	int3 AccDimensions;
	Accumulated.GetDimensions(AccDimensions.x, AccDimensions.y, AccDimensions.z);

	float acc = 0;
	for (int z = 0; z < AccDimensions.z; z++) {
		float3 enterCell = TransformFromAccToWorld(float3(DTid.xy + 0.5, z) / AccDimensions);
		float3 exitCell = TransformFromAccToWorld(float3(DTid.xy + 0.5, z + 1) / AccDimensions);
		double stepSize = length(enterCell - exitCell);

		acc += stepSize * SampleDensity(((enterCell + exitCell) * 0.5 - bMin) / (bMax - bMin));
		Accumulated[int3(DTid.xy, z)] = acc;
	}
}