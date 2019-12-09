
struct Vertex {
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

struct VS_IN {
    uint Index : SV_VertexID;
};

struct PS_IN {
    float4 Projected: SV_POSITION;
    float3 Position: POSITION;
};

cbuffer CameraTransforms : register(b0) {
    row_major matrix Projection;
    row_major matrix View;
}

StructuredBuffer<Vertex> wsVertices : register(t0);

PS_IN main(VS_IN input)
{
    int vertexId = input.Index;
    Vertex v = wsVertices[vertexId];

    float4x4 viewProjection = mul(View, Projection);

    PS_IN output = (PS_IN)0;
    output.Projected = mul(float4(v.P, 1), viewProjection);
    output.Position = float4(v.P, 1);

    return output;
}