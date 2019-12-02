/// Represents a common shader header for libraries will produce accumulative outputs
/// Requires:
/// A constant buffer with an integer of accumulated frames at ACCUMULATIVE_CB_REG register
/// Provides a function to accumulate a value at specific image coordinate

#include "CommonOutput.h"
#include "../CommonGI/Parameters.h"

cbuffer AccumulativeInfo : register(ACCUMULATIVE_CB_REG) {
	int PassCount;
}

Texture2D<float3> Background : register(BACKGROUND_IMG_REG);

RWTexture2D<float3> Accum : register(ACCUM_IMAGE_REG);

void AccumulateOutput(uint2 coord, float3 value, int s = 0, int N = 1) {
	Accum[coord] += value;

	Output[coord] =
#ifdef SHOW_DIRECT
		Background[coord] + 
#endif
#ifdef SHOW_INDIRECT
		Accum[coord] / (PassCount*N + s + 1) +
#endif
		0;// (Output[coord] * PassCount + value) / (PassCount + 1);
}