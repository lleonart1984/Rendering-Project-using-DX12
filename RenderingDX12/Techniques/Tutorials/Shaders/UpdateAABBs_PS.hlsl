struct AABB {
	float3 minim;
	float3 maxim;
};

RWStructuredBuffer<AABB> Boxes : register(u0);

#define RES 128

void main( float4 proj : SV_POSITION)
{
	int i = (uint) proj.y;
	int j = (uint) proj.x;
	float time = 10;
	float x = -0.5 + i * 1.0f / RES+0.2f*sin(5*3.14f*(float)j / RES);
	float y = -0.5 + j * 1.0f / RES + 0.5f*cos(3 * time * 0.1f + 31.14f*(float)i / RES);
	Boxes[j * RES + i].minim = float3(x, y, 0.0f);
	Boxes[j * RES + i].maxim = float3(x + 3.5f / RES, y + 3.5f / RES, 1.0f / RES);
}