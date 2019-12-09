#include "../../Common/CS_Constants.h"

struct MinMax {
    int3 min;
    int3 max;
};

cbuffer ComputeShaderInfo : register(b0)
{
    uint3 InputSize;
}

//RWStructuredBuffer<int3> sceneBoundaries : register (u1);
RWStructuredBuffer<MinMax> boundaries : register(u0);

groupshared MinMax sdata[CS_GROUPSIZE_1D];

[numthreads(CS_GROUPSIZE_1D, 1, 1)]
void main(uint3 threadIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
    unsigned int tid = threadIdx.x;
    unsigned int i = tid + groupIdx.x * (CS_GROUPSIZE_1D << 1);
    MinMax infinity;
    infinity.min = 1000000;
    infinity.max = -1000000;

    if (i >= InputSize.x) {
        sdata[tid] = infinity;
    }
    else {
        sdata[tid].min = min(boundaries[i].min, boundaries[i + CS_GROUPSIZE_1D].min);
        sdata[tid].max = max(boundaries[i].max, boundaries[i + CS_GROUPSIZE_1D].max);
    }
    
    GroupMemoryBarrierWithGroupSync();

    if (CS_GROUPSIZE_1D >= 512) {
        if (tid < 256) {
            sdata[tid].min = min(sdata[tid].min, sdata[tid + 256].min);
            sdata[tid].max = max(sdata[tid].max, sdata[tid + 256].max);
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (CS_GROUPSIZE_1D >= 256) {
        if (tid < 128) {
            sdata[tid].min = min(sdata[tid].min, sdata[tid + 128].min);
            sdata[tid].max = max(sdata[tid].max, sdata[tid + 128].max);
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (CS_GROUPSIZE_1D >= 128) {
        if (tid < 64) {
            sdata[tid].min = min(sdata[tid].min, sdata[tid + 64].min);
            sdata[tid].max = max(sdata[tid].max, sdata[tid + 64].max);
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (tid < 32) {
        sdata[tid].min = min(sdata[tid].min, sdata[tid + 32].min);
        sdata[tid].max = max(sdata[tid].max, sdata[tid + 32].max);
        sdata[tid].min = min(sdata[tid].min, sdata[tid + 16].min);
        sdata[tid].max = max(sdata[tid].max, sdata[tid + 16].max);
        sdata[tid].min = min(sdata[tid].min, sdata[tid + 8].min);
        sdata[tid].max = max(sdata[tid].max, sdata[tid + 8].max);
        sdata[tid].min = min(sdata[tid].min, sdata[tid + 4].min);
        sdata[tid].max = max(sdata[tid].max, sdata[tid + 4].max);
        sdata[tid].min = min(sdata[tid].min, sdata[tid + 2].min);
        sdata[tid].max = max(sdata[tid].max, sdata[tid + 2].max);
        sdata[tid].min = min(sdata[tid].min, sdata[tid + 1].min);
        sdata[tid].max = max(sdata[tid].max, sdata[tid + 1].max);
    }

    if (tid == 0) {
        boundaries[groupIdx.x] = sdata[0];
    }

    /*int3 a = boundaries[i].min;
    int3 b = boundaries[i].max;

    InterlockedMin(sceneBoundaries[0].x, a.x);
    InterlockedMin(sceneBoundaries[0].y, a.y);
    InterlockedMin(sceneBoundaries[0].z, a.z);
    InterlockedMax(sceneBoundaries[1].x, b.x);
    InterlockedMax(sceneBoundaries[1].y, b.y);
    InterlockedMax(sceneBoundaries[1].z, b.z);*/
}