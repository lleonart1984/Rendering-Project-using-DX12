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

void main(PS_IN input, out int triangleIndex : SV_TARGET0, out float3 coordinates : SV_TARGET1)
{
    float3 P = input.P;
    triangleIndex = input.TriangleIndex;

    float3 P0 = Vertexes[triangleIndex * 3 + 0].P;
    float3 P1 = Vertexes[triangleIndex * 3 + 1].P;
    float3 P2 = Vertexes[triangleIndex * 3 + 2].P;

    float3 N = cross(P1 - P0, P2 - P0);

    float a = abs(dot(cross(N, P2 - P1), P - P1));
    float b = abs(dot(cross(N, P2 - P0), P - P0));
    float c = abs(dot(cross(N, P1 - P0), P - P0));

    float l = a + b + c;

    coordinates = l == 0 ? 0 : float3(a, b, c) / l;
}
