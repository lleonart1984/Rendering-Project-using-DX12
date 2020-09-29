/// Represents a common shader header for libraries will produce accumulative outputs
/// Requires:
/// A constant buffer with an integer of accumulated frames at ACCUMULATIVE_CB_REG register
/// Provides a function to accumulate a value at specific image coordinate

#include "CommonOutput.h"
#include "../CommonGI/Parameters.h"

cbuffer AccumulativeInfo : register(ACCUMULATIVE_CB_REG) {
	int PassCount;
	int AccumulationIsComplexity;
}

//float3 GetColor(int complexity) {
//
//	if (complexity == 0)
//		return float3(0, 0, 0);
//
//	//return float3(1,1,1);
//
//	float level = log2(complexity);
//	float3 stopPoints[11] = {
//		float3(0,0,0.5), // 1
//		float3(0,0,1), // 2
//		float3(0,0.5,1), // 4
//		float3(0,1,1), // 8
//		float3(0,1,0.5), // 16
//		float3(0,1,0), // 32
//		float3(0.5,1,0), // 64
//		float3(1,1,0), // 128
//		float3(1,0.5,0), // 256
//		float3(1,0,0), // 512
//		float3(1,0,1) // 1024
//	};
//
//	if (level >= 10)
//		return stopPoints[10];
//
//	return lerp(stopPoints[(int)level], stopPoints[(int)level + 1], level % 1);
//}

#include "CommonComplexity.h"

Texture2D<float3> Background : register(BACKGROUND_IMG_REG);

RWTexture2D<float3> Accum : register(ACCUM_IMAGE_REG);

float3 filter(float3 color) {
	return color;
	//return (color.x + color.y + color.z) / 3 > 0.5 ? 1 : 0;
}

void AccumulateOutput(uint2 coord, float3 value, int s = 0, int N = 1) {
	Accum[coord] += value;

	if (AccumulationIsComplexity) {

		float3 acc = Accum[coord] / (PassCount * N + s + 1);

		Output[coord] = GetColor((int)(acc.x * 256 + acc.y));
	}
	else {
		Output[coord] = filter(
#ifdef SHOW_DIRECT
			Background[coord] +
#endif
#ifdef SHOW_INDIRECT
			Accum[coord] / (PassCount * N + s + 1) +
#endif
			0);// (Output[coord] * PassCount + value) / (PassCount + 1);
	}
}