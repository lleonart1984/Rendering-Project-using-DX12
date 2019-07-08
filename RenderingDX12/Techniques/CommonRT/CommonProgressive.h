/// Represents a common shader header for libraries will produce accumulative outputs
/// Requires:
/// A constant buffer with an integer of accumulated frames at ACCUMULATIVE_CB_REG register
/// Provides a function to accumulate a value at specific image coordinate

#include "CommonOutput.h"

cbuffer AccumulativeInfo : register(ACCUMULATIVE_CB_REG) {
	int PassCount;
}

void AccumulateOutput(uint2 coord, float3 value) {
	Output[coord] = (Output[coord] * PassCount + value) / (PassCount + 1);
}