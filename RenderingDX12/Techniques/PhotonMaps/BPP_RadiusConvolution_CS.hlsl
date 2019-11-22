#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION

#include "PhotonDefinition.h"
#include "../CommonGI/Parameters.h"

StructuredBuffer<Photon> Photons		: register (t0);
StructuredBuffer<float> In_Radii		: register (t1);
RWStructuredBuffer<float> Out_Radii		: register (u0);

[numthreads(CS_1D_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	int index = DTid.x;

	float outputRadius = 0;

	if (any(Photons[index].Intensity))
	{
		float total = In_Radii[index];
		float kernelInt = 1;

		int CHECK = 128;

		for (int i = 1; i <= CHECK; i += 1)
		{
			float kernel = 2 * (1 - i / (float)CHECK);
			int hasR = any(Photons[index + i].Intensity);
			int hasL = any(Photons[index - i].Intensity);
			total += (In_Radii[index + i] * hasR + In_Radii[index - i] * hasL) * kernel;
			kernelInt += (hasR + hasL) * kernel;
		}

		outputRadius = total / kernelInt;
	}

	Out_Radii[index] = outputRadius;
	//Out_Radii[index] = In_Radii[index];
}