cbuffer Globals : register(b0) {
    row_major matrix Projection;
};

struct PS_IN
{
    float4 Proj : SV_POSITION;
    float3 P : POSITION;
    nointerpolation uint TriangleIndex : TRIANGLEINDEX;
};

struct TransformedVertex
{
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

StructuredBuffer<TransformedVertex> Vertexes : register (t0);

PS_IN main(uint index : SV_VertexID)
{
    PS_IN output = (PS_IN)output;
    output.P = Vertexes[index].P;
    output.Proj = mul(float4(output.P, 1), Projection);
    output.TriangleIndex = index / 3;
    return output;
}
