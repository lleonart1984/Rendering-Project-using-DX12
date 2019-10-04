
struct PS_IN {
    float4 P: SV_POSITION;
    float2 C: TEXCOORD;
};

struct Fragment {
    int Index;
    float Min;
    float Max;
};

cbuffer ScreenInfo : register (b0) {
    int Width;
    int Height;
}

StructuredBuffer<int3> sceneBoundaries  : register(t0);
StructuredBuffer<Fragment> fragments  : register(t1);
Texture2D<int> rootBuffer             : register(t2);
StructuredBuffer<int> nextBuffer      : register(t3);

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

float4 main(PS_IN input) : SV_TARGET
{
    int2 dimensions = int2(Width, Height);
    int2 pxy = input.C.xy * dimensions;

    //return float4(GetColor(pxy.x + pxy.y), 1);
    int count = 1;
    int current = rootBuffer[pxy];
    /*if (current == -1) {
        return float4(1, 0.5, 0, 1);
    }
    else {
        return float4(0, 0.5, 0.3, 1);
    }*/
    while (current != -1) {
        current = nextBuffer[current];
        count++;
    }
    
    return float4(GetColor(count), 1);

    /*float3 lowerCorner = sceneBoundaries[0] / 1e7;
    float3 upperCorner = sceneBoundaries[1] / 1e7;
    float3 unit = upperCorner - lowerCorner;

    float3 d = input.Position - lowerCorner;
    
    return float4(d / unit, 1);*/
}