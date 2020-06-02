#include "../CommonGI/Parameters.h"

Texture2D<ELEMENT_TYPE> Input : register(INPUT_REG);
Texture2D<ELEMENT_TYPE> Accumulation : register(ACCUMULATION_REG);

RWTexture2D<ELEMENT_TYPE> Output : register(OUTPUT_REG);

int getNumberOfSamples();

int2 getSampleOffset(int sampleID);

float Kernel(int2 currentPx, float variance, int sampleID, int2 sampleCoord);

float Variance(int2 currentPx) {

	float sqrSum = 0;
	float sum = 0;

	for (int dx = -2; dx <= 2; dx++)
		for (int dy = -2; dy <= 2; dy++)
		{
			int2 sampleCoord = int2(dx, dy) + currentPx;
			float sqrValue = dot(Accumulation[sampleCoord], Accumulation[sampleCoord]);
			float value = sqrt(sqrValue);
			sqrSum += sqrValue;
			sum += value;
		}

	float m = sum / 25.0;

	return sqrt(max(0, sqrSum / 25.0 - m * m));
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int2 currentPx = DTid.xy;

	int2 dim;
	Input.GetDimensions(dim.x, dim.y);

	if (!all(currentPx < dim))
		return;

	float kernelIntegral = 0;
	ELEMENT_TYPE acc = 0;

	float variance = Variance(currentPx);

	for (int i = 0; i < getNumberOfSamples(); i++) {
		int2 sampleCoord = getSampleOffset(i) + currentPx;
		float kernel = Kernel(currentPx, variance, i, sampleCoord);
		acc += kernel * Input[sampleCoord];
		kernelIntegral += kernel;
	}

	if (kernelIntegral > 0)
		acc /= kernelIntegral;

	Output[currentPx] = acc;
}

