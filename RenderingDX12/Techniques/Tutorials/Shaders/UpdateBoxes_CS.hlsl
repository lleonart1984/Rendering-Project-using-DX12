
struct AABB {
	float3 minim;
	float3 maxim;
};

cbuffer Timing : register(b0) {
	int Time;
};

RWStructuredBuffer<AABB> Boxes : register(u0);

#define RES 128

[numthreads(32, 32, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int i = DTid.y;
	int j = DTid.x;
	float time = Time * 0.1;
	float x = -0.5 + i * 1.0f / RES + 0.2f*sin(5 * 3.14f*(float)j / RES);
	float y = -0.5 + j * 1.0f / RES + 0.5f*cos(3 * time * 0.1f + 31.14f*(float)i / RES);
	Boxes[j * RES + i].minim = float3(x, y, 0.0f);
	Boxes[j * RES + i].maxim = float3(x + 3.5f / RES, y + 3.5f / RES, 1.0f / RES);
}
