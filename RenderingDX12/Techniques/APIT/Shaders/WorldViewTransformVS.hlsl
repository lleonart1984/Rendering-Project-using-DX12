/// Transforms vertex data in view-space for a future GS projection into pixels.

struct VS_IN
{
    float3 P: POSITION;
    float3 N: NORMAL;
    float2 C: TEXCOORD;
};

struct VS_OUT {
    float3 P: POSITION;
    float3 N: NORMAL;
    float2 C: TEXCOORD;
};

cbuffer CameraTransforms : register(b0) {
    row_major matrix Projection;
    row_major matrix View;
}

StructuredBuffer<VS_IN> vertices        : register(t0);
StructuredBuffer<int> objects           : register(t1);
StructuredBuffer<float4x4> transforms   : register(t2);

VS_OUT main(uint vertexId: SV_VertexID)
{
    VS_IN v = vertices[vertexId];
    int objectId = objects[vertexId];
    float4x4 world = transforms[objectId];

    VS_OUT output = (VS_OUT)0;

    float4x4 worldView = mul(World, View);
    output.P = mul(float4(input.P, 1), worldView).xyz; // Multiplicar por Projection?
    output.N = normalize(mul(float4 (input.N, 0), worldView).xyz);
    output.C = input.C;

    return output;
}
