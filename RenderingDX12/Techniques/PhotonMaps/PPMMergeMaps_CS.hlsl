#include "../CommonGI/Parameters.h"

struct RayHitAccum {
	float N;
	float3 Accum;
};

Texture2D<int> ScreenHead : register(t0);
StructuredBuffer<int> ScreenNext : register(t1);
StructuredBuffer<RayHitAccum> WPM : register (t2);

RWStructuredBuffer<RayHitAccum> TPM : register (u0);
RWStructuredBuffer<float> Radii		: register (u1);

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int2 dim;
	ScreenHead.GetDimensions(dim.x, dim.y);

	int2 px = DTid.xy;

	if (all(px < dim)) {
		int currentNode = ScreenHead[px];

		while (currentNode != -1) {

			float3 acc1 = TPM[currentNode].Accum;
			float3 acc2 = WPM[currentNode].Accum;
			float N = TPM[currentNode].N;
			float M = WPM[currentNode].N;

			float alpha = 0.7;// min(1, pow(0.9, (double)M / DESIRED_PHOTONS));

			if (N == 0.0) // initial wave 
			{
				float ratio = clamp(sqrt(DESIRED_PHOTONS / M), 0.0125, 1);
				TPM[currentNode].N = WPM[currentNode].N * ratio;
				TPM[currentNode].Accum = WPM[currentNode].Accum * ratio;

				Radii[currentNode] *= sqrt(ratio);
			}
			else {
				float nN = N + alpha * M;
				float ratio = nN / (N + M);

				TPM[currentNode].N = nN;
				TPM[currentNode].Accum = (acc1 + acc2) * ratio;

				Radii[currentNode] *= sqrt(ratio);
			}

			currentNode = ScreenNext[currentNode];
		}
	}
}