cbuffer ScreenInfo : register (b0) {
    int Width;
    int Height;

    int pWidth;
    int pHeight;

    int Levels;
}

//RWStructuredBuffer<Fragment> fragments  : register(u0);
// index of root node of each pixel
Texture2D<int> firstBuffer            : register(t0);
// References to next fragment node index for each node in linked list
StructuredBuffer<int> nextBuffer      : register(t1);

float3 GetColor(int complexity) {
    if (complexity == 0) {
        return float3(1, 1, 1);
    }

    float level = log2(complexity);
    float3 stopPoints[11] = {
        float3(0,0,0.5), // 1
        float3(0,0,1), // 2
        float3(0,0.5,1), // 4
        float3(0,1,1), // 8
        float3(0,1,0.5), // 16
        float3(0,1,0), // 32
        float3(0.5,1,0), // 64
        float3(1,1,0), // 128
        float3(1,0.5,0), // 256
        float3(1,0,0), // 512
        float3(1,0,1) // 1024
    };

    if (level >= 10) {
        return stopPoints[10];
    }

    return lerp(stopPoints[(int)level], stopPoints[(int)level + 1], level % 1);
}

float4 main(float4 P: SV_POSITION) : SV_TARGET
{
    float2 pNorm = P.xy / float2(Height, Height);
    //return float4(P.xy / float2(Width, Height), 0, 1);
    float fpx = (pNorm.x % 0.3333333) * 3;
    float fpy = (pNorm.y % 0.25) * 4;
    int col = pNorm.x * 3;
    int row = pNorm.y * 4;

    int faceIndex = -1;
    if (row == 1 && col == 2)
        faceIndex = 0; // positive x

    if (row == 1 && col == 0)
        faceIndex = 1; // negative x

    if (row == 1 && col == 1)
        faceIndex = 5; // negative z

    if (row == 3 && col == 1)
        faceIndex = 4; // positive z

    if (row == 0 && col == 1)
        faceIndex = 2; // positive y

    if (row == 2 && col == 1)
        faceIndex = 3; // negative y

    if (faceIndex == -1)
        return float4(0, 0, 0, 1);

    uint2 coord = uint2(fpx * Width + Width * faceIndex, fpy * Height);

    int idx = firstBuffer[coord];
    int count = 0;
    while (idx != -1) {
        count += 1;
        idx = nextBuffer[idx];
    }

    if (count == 0) {
        return float4(0, 0, 0, 1);
    }

    return float4(GetColor(count), 1);
}