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
		float3 N = Photons[index].Normal;

		float total = In_Radii[index] * 2;
		float kernelInt = 2;

		int CHECK = 113 + index % 123;

		for (float i = 1; i <= CHECK; i += 1)
		{
			float kernel = 2 * (1 - i / (float)CHECK);
			float hasR = any(Photons[index + i].Intensity) * max(0, dot(N, Photons[index + i].Normal));
			float hasL = any(Photons[index - i].Intensity) * max(0, dot(N, Photons[index - i].Normal));
			total += (In_Radii[index + i] * hasR + In_Radii[index - i] * hasL) * kernel;
			kernelInt += (hasR + hasL) * kernel;
		}

		outputRadius = total / kernelInt;
	}

	//Out_Radii[index] = outputRadius;
	Out_Radii[index] = In_Radii[index];
}