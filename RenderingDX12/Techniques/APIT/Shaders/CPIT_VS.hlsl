struct Vertex {
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

StructuredBuffer<Vertex> VSVB : register(t0); // Vertex buffer in View Space

struct GS_IN {
    float3 Position : POSITION;
    uint VertexIndex : VERTEXINDEX;
};

struct VS_IN {
    uint Index : SV_VertexID;
};

GS_IN main(VS_IN input)
{
    int index = input.Index;

    GS_IN output = (GS_IN)0;
    output.Position = VSVB[index].P;
    output.VertexIndex = index;

    return output;
}
