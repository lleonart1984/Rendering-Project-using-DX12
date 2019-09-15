struct Vertex {
    float3 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 Coordinates  : TEXCOORD;
    float3 Tangent      : TANGENT;
    float3 Binormal     : BINORMAL;
};

struct VS_IN {
    uint Index : SV_VertexID;
};

struct PS_IN {
    float4 Projected    : SV_POSITION;
    int VertexIndex : VERTEXINDEX;
    Vertex vertex;
};

cbuffer CameraTransforms : register(b0) {
    row_major matrix View;
}

StructuredBuffer<Vertex> vertices : register(t0);
StructuredBuffer<int> objects           : register(t1);
StructuredBuffer<float4x4> transforms   : register(t2);

PS_IN main(VS_IN input)
{
    int vertexId = input.Index;
    Vertex v = vertices[vertexId];
    int objectId = objects[vertexId];
    float4x4 world = transforms[objectId];

    PS_IN output = (PS_IN)0;

    output.Projected = float4(0, 0, 0.5, 1);
    output.VertexIndex = vertexId;

    float4x4 worldView = mul(world, View);
    output.vertex.Position = mul(float4(v.Position, 1), worldView).xyz;
    output.vertex.Normal = normalize(mul(float4(v.Normal, 0), worldView).xyz);
    output.vertex.Coordinates = v.Coordinates;
    output.vertex.Tangent = normalize(mul(float4(v.Tangent, 0), worldView).xyz);
    output.vertex.Binormal = normalize(mul(float4(v.Binormal, 0), worldView).xyz);

    return output;
}