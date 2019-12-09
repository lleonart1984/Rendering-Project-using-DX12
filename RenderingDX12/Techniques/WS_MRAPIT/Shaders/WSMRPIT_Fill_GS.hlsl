
struct GS_IN {
    float3 P : POSITION;
    uint VertexIndex : VERTEXINDEX;
};

struct GS_OUT {
    float4 proj : SV_POSITION;
    float3 P0 : POSITION0;
    float3 P1 : POSITION1;
    float3 P2 : POSITION2;
    int TriangleIndex : TRIANGLEINDEX;
    nointerpolation uint Level : RESOLUTIONLEVEL;
};

cbuffer ScreenInfo : register (b0)
{
    int Width;
    int Height;
    int Levels;
}

cbuffer Reticulation : register (b1)
{
    int K;
    int D;
}

[maxvertexcount(4)]
void main(triangle GS_IN input[3], inout TriangleStream<GS_OUT> output)
{
    GS_OUT gsOut = (GS_OUT)0;
    gsOut.P0 = input[0].P;
    gsOut.P1 = input[1].P;
    gsOut.P2 = input[2].P;
    gsOut.TriangleIndex = input[0].VertexIndex / 3;

    // Compute Corners
    float2 minim = min(input[0].P.xy, min(input[1].P.xy, input[2].P.xy));
    float2 maxim = max(input[0].P.xy, max(input[1].P.xy, input[2].P.xy));

    // Compute level of resolution
    float dimensions = float2(Width, Height);
    float2 deltaDistance = (maxim - minim) * dimensions / 2;
    float u = max(deltaDistance.x, deltaDistance.y);
    float l = min(deltaDistance.x, deltaDistance.y);
    float clampedMin = max(l, u / K);
    int level = max(0, int(log2(clampedMin)) - D);
    //level = min(Levels, level);
    
    gsOut.Level = level;

    // Readjust the coordinates for the level
    minim = (minim + float2(1, -1)) / (1 << level) - float2(1, -1);
    maxim = (maxim + float2(1, -1)) / (1 << level) - float2(1, -1);

    // Offset of a half pixel
    float2 halfPixelSize = (1 << level) / dimensions;
    minim -= halfPixelSize;
    maxim += halfPixelSize;
    minim = max(float2(-1, -1), minim);
    maxim = min(float2(1, 1), maxim);

    // Project quad
    gsOut.proj = float4(minim.x, minim.y, 0.5, 1);
    output.Append(gsOut);

    gsOut.proj = float4(maxim.x, minim.y, 0.5, 1);
    output.Append(gsOut);

    gsOut.proj = float4(minim.x, maxim.y, 0.5, 1);
    output.Append(gsOut);

    gsOut.proj = float4(maxim.x, maxim.y, 0.5, 1);
    output.Append(gsOut);
}