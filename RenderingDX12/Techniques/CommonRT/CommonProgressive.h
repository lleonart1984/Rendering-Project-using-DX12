/// Represents a common shader header for libraries will produce accumulative outputs
/// Requires:
/// A constant buffer with an integer of accumulated frames at ACCUMULATIVE_CB_REG register
/// Provides a function to accumulate a value at specific image coordinate

#include "CommonOutput.h"

cbuffer AccumulativeInfo : register(ACCUMULATIVE_CB_REG) {
	int PassCount;
}

RWTexture2D<float3> Accum : register(ACCUM_IMAGE_REG);

void AccumulateOutput(uint2 coord, float3 value) {
	Accum[coord] += value;
	Output[coord] = Accum[coord] / (PassCount + 1);// (Output[coord] * PassCount + value) / (PassCount + 1);
}