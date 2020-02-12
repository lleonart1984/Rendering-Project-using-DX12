#include "../CommonGI/Parameters.h"

struct PSInput
{
	float4 Proj : SV_POSITION;
	float3 X	: POSITION;
	float3 P	: PHOTON_POSITION;
	float3 I	: PHOTON_CONTRIBUTION;
	float  K	: SPLAT_KERNEL;
};

float3 main(PSInput input) : SV_TARGET
{
	float radius = PHOTON_RADIUS;
	float d = length(input.X - input.P);

	if (d > radius)
		return 0;

	float kernel = input.K * 758/1024.0;// (1 - d / radius) * 6 / 3.14159;// input.K;

	return input.I * kernel;
}