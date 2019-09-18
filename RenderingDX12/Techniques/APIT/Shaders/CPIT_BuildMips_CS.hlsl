#include "../../Common/CS_Constants.h"

cbuffer BuildingMipsInfo : register(b0) {
    int Resolution;
    int Level;
};

StructuredBuffer<int> Morton : register(t0);
StructuredBuffer<int> StartMipMaps : register(t1);

RWStructuredBuffer<float2> MipMaps : register(u0);

int get_index(int2 px, int view, int level)
{
    int res = Resolution >> level;
    return StartMipMaps[level] + Morton[px.x] + Morton[px.y] * 2 + view * res * res;
}

[numthreads(CS_GROUPSIZE_2D, CS_GROUPSIZE_2D, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int2 px = int2(DTid.xy);
    int res = Resolution >> Level;
    int view = px.x / res;
    px.x %= res;

    int currentIndex = get_index(px, view, Level);
    int firstChildIndex = get_index(px << 1, view, Level - 1);
    MipMaps[currentIndex] = float2(
        min(
            min(MipMaps[firstChildIndex].x, MipMaps[firstChildIndex + 1].x),
            min(MipMaps[firstChildIndex + 2].x, MipMaps[firstChildIndex + 3].x)),
        max(
            max(MipMaps[firstChildIndex].y, MipMaps[firstChildIndex + 1].y),
            max(MipMaps[firstChildIndex + 2].y, MipMaps[firstChildIndex + 3].y))
        );
}