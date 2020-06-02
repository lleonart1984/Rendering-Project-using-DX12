#define ELEMENT_TYPE float4
#define INPUT_REG t0
#define OUTPUT_REG u0

cbuffer WaveletInfo : register(b0) {
	int Size;
	int Pass;
};

Texture2D<float4> Radiances			: register(t1); //initial radiances XYZ density=W
Texture2D<float4> Positions			: register(t2); //position XYZ position pdf=W
Texture2D<float4> Directions		: register(t3); //direction XYZ direction pdf=W
Texture2D<float3> Gradient			: register(t4); // density gradient at scattered position

#include "Convolution.h"

int getNumberOfSamples() {
	return 25;
}

int2 getSampleOffset(int sampleID) {
	int row = sampleID / 5;
	int col = sampleID % 5;
	return int2(row - 2, col - 2) * Size;

	/*int2 offsets[] = { 
		int2(0,0), 
		int2(0,-1), int2(1,0), int2(0,1), int2(-1,0), 
		int2(-1,-1), int2(-1,1), int2(1,1), int2(1,-1) 
	};
	return offsets[sampleID] * Size;*/
}

float DirectionWeight(float3 v1, float3 v2) {

	return max(0.0001, pow(dot(v1, v2), 0.1));
}

float Kernel(int2 currentPx, int sampleID, int2 sampleCoord) {
	//if (Positions[currentPx].w == 0)
		return sampleID == 12;

	// Gaussian 5x5
	float kernels[] = {
		1.0, 4.0, 6.0, 4.0, 1.0,
		4.0, 16.0, 24.0, 16.0, 4.0,
		6.0, 24.0, 36.0, 24.0, 6.0,
		4.0, 16.0, 24.0, 16.0, 4.0,
		1.0, 4.0, 6.0, 4.0, 1.0
	};

	float passPower = Pass;

	float wd = 1;// exp(-abs(Positions[currentPx].w - Positions[sampleCoord].w) * Size);
	float wp = 1;// exp(-length(Positions[currentPx].xyz - Positions[sampleCoord].xyz) * (100));
	float wg = 1;// DirectionWeight(Gradient[currentPx], Gradient[sampleCoord]);// pow(saturate(dot(Gradient[currentPx], Gradient[sampleCoord])), 2);

	return (kernels[sampleID] / 36.0) * (Radiances[sampleCoord].w > 0)* wd* wp* wg;
}
