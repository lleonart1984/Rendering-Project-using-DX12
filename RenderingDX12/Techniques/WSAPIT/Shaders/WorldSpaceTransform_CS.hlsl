#include "../../Common/CS_Constants.h"
#define PRECISION 1e7
#define EPSILON 0.001

struct Vertex
{
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

cbuffer ComputeShaderInfo : register(b0)
{
    uint3 InputSize;
}

StructuredBuffer<Vertex> vertices       : register(t0);
StructuredBuffer<int> objects           : register(t1);
StructuredBuffer<float4x4> transforms   : register(t2);

RWStructuredBuffer<Vertex> transformedVertices  : register(u0);
RWStructuredBuffer<int3> sceneBoundaries        : register(u1);

[numthreads(CS_GROUPSIZE_1D, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x >= InputSize.x) {
        return;
    }

    int vertexIndex = DTid.x;
    Vertex v = vertices[vertexIndex];
    int objectId = objects[vertexIndex];
    float4x4 world = transforms[objectId];

    Vertex output = (Vertex)0;

    output.P = mul(float4(v.P, 1), world).xyz;
    output.N = normalize(mul(float4(v.N, 0), world).xyz);
    output.C = v.C;
    output.T = normalize(mul(float4(v.T, 0), world).xyz);
    output.B = normalize(mul(float4(v.B, 0), world).xyz);
    transformedVertices[vertexIndex] = output;

    int3 lowerBound = int3((output.P - EPSILON) * PRECISION);
    int3 upperBound = int3((output.P + EPSILON) * PRECISION);

    InterlockedMin(sceneBoundaries[0].x, lowerBound.x);
    InterlockedMin(sceneBoundaries[0].y, lowerBound.y);
    InterlockedMin(sceneBoundaries[0].z, lowerBound.z);
    InterlockedMax(sceneBoundaries[1].x, upperBound.x);
    InterlockedMax(sceneBoundaries[1].y, upperBound.y);
    InterlockedMax(sceneBoundaries[1].z, upperBound.z);
}