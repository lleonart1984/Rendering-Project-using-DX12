
struct Fragment {
    int Index;
    float Min;
    float Max;
};

struct PITNode {
    int Parent;
    int LeftChild;
    int RightChild;

    float Discriminant;
};

struct PS_IN {
    float4 P: SV_POSITION;
    float2 C: TEXCOORD;
};

cbuffer ScreenInfo : register (b0) {
    int Width;
    int Height;
}

StructuredBuffer<int3> sceneBoundaries : register(t0);
StructuredBuffer<Fragment> fragments    : register(t1);
Texture2D<int> rootBuffer               : register(t2);
StructuredBuffer<PITNode> nodeBuffer    : register(t3);
StructuredBuffer<float4> boundaryBuffer : register(t4);
StructuredBuffer<int> firstBuffer       : register(t5);
StructuredBuffer<int> nextBuffer        : register(t6);
StructuredBuffer<int> preorderBuffer    : register(t7);
StructuredBuffer<int> skipBuffer        : register(t8);

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

float4 GetDensityPerNode(uint2 pxy)
{
    int nodeCount = 0;
    int fragmentCount = 0;
    int currentNode = rootBuffer[pxy];
    int maxFragsInANode = 0;

    for (int i = 0; i < 5 && currentNode != -1; i++)
    {
        while (currentNode != -1)
        {
            nodeCount++;
            int currentFragment = firstBuffer[currentNode];
            int nodeFrags = 0;
            while (currentFragment != -1)
            {
                nodeFrags++;
                fragmentCount++;
                currentFragment = nextBuffer[currentFragment];
            }

            maxFragsInANode = max(maxFragsInANode, nodeFrags);
            currentNode = preorderBuffer[currentNode];
        }
    }

    float average = fragmentCount / (float)nodeCount;

    // Variance
    currentNode = rootBuffer[pxy];
    float difference = 0;

    while (currentNode != -1)
    {
        int currentFragment = firstBuffer[currentNode];
        int nodeFrags = 0;
        while (currentFragment != -1)
        {
            nodeFrags++;
            currentFragment = nextBuffer[currentFragment];
        }

        difference += abs(nodeFrags - average);
        currentNode = preorderBuffer[currentNode];
    }

    if (nodeCount == 0)
        return float4(1, 0, 0.5, 1);

    return float4(GetColor(fragmentCount / nodeCount), 1);
}

float4 main(PS_IN input) : SV_TARGET
{
    int2 dimensions = int2(Width, Height);
    int2 pxy = input.C.xy * dimensions;

    return GetDensityPerNode(pxy);
    //return float4(GetColor(pxy[0] + pxy.y), 1);

    /*float maxDepth = boundaryBuffer[currentNode].w;
    float minDepth = boundaryBuffer[currentNode].z;

    return float4((maxDepth - minDepth) * float3(0.00, 0.27, 0.29), 1);*/

    

    /*float3 lowerCorner = sceneBoundaries[0] / 1e7;
    float3 upperCorner = sceneBoundaries[1] / 1e7;
    float3 unit = upperCorner - lowerCorner;

    float3 d = input.Position - lowerCorner;

    return float4(d / unit, 1);*/
}