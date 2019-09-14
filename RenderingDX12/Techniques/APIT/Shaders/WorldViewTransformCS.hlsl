struct Vertex
{
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

cbuffer CameraTransforms : register(b0) {
    row_major matrix View;
}

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<int> objects           : register(t1);
StructuredBuffer<float4x4> transforms   : register(t2);

RWStructuredBuffer<Vertex> transformedVertices : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    int vertexId = DTid.x;
    Vertex v = vertices[vertexId];
    int objectId = objects[vertexId];
    float4x4 world = transforms[objectId];

    Vertex output = (Vertex)0;

    float4x4 worldView = mul(world, View);
    output.P = mul(float4(v.P, 1), worldView).xyz;
    output.N = normalize(mul(float4(v.N, 0), worldView).xyz);
    output.C = v.C;
    output.T = normalize(mul(float4(v.T, 0), worldView).xyz);
    output.B = normalize(mul(float4(v.B, 0), worldView).xyz);

    transformedVertices[vertexId] = output;
}
