
struct Vertex {
    float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 Coordinates  : TEXCOORD;
    float3 Tangent      : TANGENT;
    float3 Binormal     : BINORMAL;
};

struct PS_IN {
    float4 Projected    : SV_POSITION;
    int VertexIndex : VERTEXINDEX;
    Vertex v;
    /*float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 Coordinates  : TEXCOORD;
    float3 Tangent      : TANGENT;
    float3 Binormal     : BINORMAL;*/
};

RWStructuredBuffer<Vertex> transformedVertices : register(u0);

void main(PS_IN input)
{
    transformedVertices[input.VertexIndex] = input.v;
}
