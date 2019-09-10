cbuffer settingMaterialIndex : register(b0)
{
    int MaterialIndexData : packoffset (c0);
}

struct GS_OUT
{
    float4 proj : SV_POSITION;

    float3 P1 : POSITION;
    float3 N1 : NORMAL;
    float2 C1 : TEXCOORD;
    float3 P2 : POSITION1;
    float3 N2 : NORMAL1;
    float2 C2 : TEXCOORD1;
    float3 P3 : POSITION2;
    float3 N3 : NORMAL2;
    float2 C3 : TEXCOORD2;
};

struct TransformedVertex
{
    float3 P;
    float3 N;
    float2 C;
    int MaterialIndex;
    int TriangleIndex;
};

RWStructuredBuffer<TransformedVertex> vertexes : register (u0);
RWStructuredBuffer<int> count : register (u1);

void PS(GS_OUT In)
{
    int currentIndex;
    InterlockedAdd(count[0], 1, currentIndex);

    int MI = MaterialIndexData;

    TransformedVertex temp = (TransformedVertex)0;

    temp.P = In.P1;
    temp.N = In.N1;
    temp.C = In.C1;
    temp.MaterialIndex = MI;
    temp.TriangleIndex = currentIndex;

    vertexes[currentIndex * 3] = temp;

    temp.P = In.P2;
    temp.N = In.N2;
    temp.C = In.C2;
    temp.MaterialIndex = MI;
    temp.TriangleIndex = currentIndex;

    vertexes[currentIndex * 3 + 1] = temp;

    temp.P = In.P3;
    temp.N = In.N3;
    temp.C = In.C3;
    temp.MaterialIndex = MI;
    temp.TriangleIndex = currentIndex;

    vertexes[currentIndex * 3 + 2] = temp;
}