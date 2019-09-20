struct AABB {
	float3 minim;
	float3 maxim;
};
StructuredBuffer<AABB> Boxes : register(t0);

RWStructuredBuffer<float3> Colors : register(u0);

#define RES 128

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	int i = DTid.y;
	int j = DTid.x;

	Colors[j * RES + i] = float3(abs(sin(Boxes[j * RES + i].minim.x * 3)), abs(cos(Boxes[j * RES + i].minim.y * 5)), 1);
	//Colors[j * RES + i] = float3(abs(sin(0)), abs(cos(0)), 1);
}