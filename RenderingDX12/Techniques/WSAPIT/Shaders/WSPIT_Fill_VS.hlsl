
#define PRECISION 1e7

struct Vertex
{
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

struct GS_IN {
    float3 NormalizedPosition : POSITION;
    uint VertexIndex : VERTEXINDEX;
};

struct VS_IN {
    uint Index : SV_VertexID;
};

StructuredBuffer<Vertex> wsVertices : register(t0);
StructuredBuffer<int3> sceneBoundaries : register(t1);

GS_IN main(VS_IN input)
{
    // Normalize Position (x, y) in [-1, 1], z in [0, 1]
    float3 lowerCorner = sceneBoundaries[0] / PRECISION;
    float3 upperCorner = sceneBoundaries[1] / PRECISION;

    int index = input.Index;
    float3 position = (wsVertices[index].P - lowerCorner) / (upperCorner - lowerCorner);
    position.xy = position.xy * 2 - 1;

    GS_IN output = (GS_IN)0;
    output.NormalizedPosition = position;
    output.VertexIndex = index;

    return output;
}