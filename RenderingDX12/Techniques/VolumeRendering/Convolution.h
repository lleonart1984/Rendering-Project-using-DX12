#include "../CommonGI/Parameters.h"

Texture2D<ELEMENT_TYPE> Input : register(INPUT_REG);

RWTexture2D<ELEMENT_TYPE> Output : register(OUTPUT_REG);

int getNumberOfSamples();

int2 getSampleOffset(int sampleID);

float Kernel(int2 currentPx, int sampleID, int2 sampleCoord);

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int2 currentPx = DTid.xy;
	float kernelIntegral = 0;
	ELEMENT_TYPE acc = 0;
	
	for (int i = 0; i < getNumberOfSamples(); i++) {
		int2 sampleCoord = getSampleOffset(i) + currentPx;
		float kernel = Kernel(currentPx, i, sampleCoord);
		acc += kernel * Input[sampleCoord];
		kernelIntegral += kernel;
	}
	
	if (kernelIntegral > 0)
		acc /= kernelIntegral;

	Output[currentPx] = acc;
}

