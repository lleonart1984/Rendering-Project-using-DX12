struct RayInfo
{
	float3 Position;
	float3 Direction;
	float3 AccumulationFactor;
};

RWStructuredBuffer<RayInfo> OutputRays		: register(u0);
RWTexture2D<int> OutputHeadBuffer			: register(u1);
RWStructuredBuffer<int> OutputNextBuffer	: register(u2);
RWStructuredBuffer<int> Malloc				: register(u3);

void YieldRay(uint2 px, float3 P, float3 D, float3 Importance) {
	RayInfo bRay = (RayInfo)0;
	bRay.Position = P;
	bRay.Direction = D;
	bRay.AccumulationFactor = Importance;

	int newIndex;
	InterlockedAdd(Malloc[0], 1, newIndex);
	OutputRays[newIndex] = bRay;
	OutputNextBuffer[newIndex] = OutputHeadBuffer[px];
	OutputHeadBuffer[px] = newIndex;
}