
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
};

cbuffer ScreenInfo : register (b0)
{
    int Width;
    int Height;
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

    // Offset of a half pixel
    float2 halfPixelSize = float2(1, 1) / float2(Width, Height);
    minim -= halfPixelSize;
    maxim += halfPixelSize;

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