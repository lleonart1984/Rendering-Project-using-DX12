struct PSInput
{
    float4 projected : SV_POSITION;
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    float2 uv : TEXCOORD;
    int objectId : OBJECTID;
};

struct Vertex
{
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

cbuffer Globals : register(b0)
{
    row_major matrix projection;
    row_major matrix view;
};

StructuredBuffer<Vertex> vertices		: register(t0);
StructuredBuffer<int> objects			: register(t1);
StructuredBuffer<float4x4> transforms	: register(t2);

PSInput main(uint vertexId: SV_VertexID)
{
    Vertex v = vertices[vertexId];
    int objectId = objects[vertexId];
    float4x4 world = transforms[objectId];

    PSInput result;

    float4x4 worldview = mul(world, view);

    result.position = v.P;
    result.normal = v.N;
    result.tangent = v.T;
    result.binormal = v.B;
    result.projected = mul(float4(result.position, 1), projection);
    result.uv = v.C;
    result.objectId = objectId;

    return result;
}
