cbuffer InverseProjection : register(b0)
{
    row_major matrix InverseProj;
};

cbuffer ScreenInfo : register(b1)
{
    int Width;
    int Height;
};

struct RayInfo
{
    float3 Position;
    float3 Direction;
    float3 AccumulationFactor;
};

struct RayIntersection
{
    int TriangleIndex;
    float3 Coordinates;
};

RWStructuredBuffer<RayInfo> Output : register(u0);
RWStructuredBuffer<RayIntersection> Intersections : register(u1);
RWTexture2D<int> HeadBuffer : register(u2);
RWStructuredBuffer<int> NextBuffer : register(u3);

Texture2D<int> InitialTriangleIndices : register(t0);
Texture2D<float3> InitialCoordinates : register(t1);

void main(float4 proj : SV_POSITION, float2 C : TEXCOORD)
{
    int2 px = proj.xy;
    float3 NPC = float3((C.x * 2 - 1), 1 - C.y * 2, -0);
    float3 dir = mul(float4(NPC, 1), InverseProj).xyz;

    int rayID = px.y * Width + px.x;

    RayInfo ray = (RayInfo)0;
    ray.AccumulationFactor = float3(1, 1, 1);
    ray.Direction = normalize(dir);

    Output[rayID] = ray;
    HeadBuffer[proj.xy] = rayID;
    NextBuffer[rayID] = -1;

    Intersections[rayID].TriangleIndex = InitialTriangleIndices[px];
    Intersections[rayID].Coordinates = InitialCoordinates[px];
}