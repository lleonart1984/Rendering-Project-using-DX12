// This constant buffer gets the information of the face
cbuffer ScreenInfo : register (b0)
{
    int Width;
    int Height;

    int pWidth;
    int pHeight;

    int Levels;
}

struct PITNode {
    int Parent;
    int LeftChild;
    int RightChild;

    float Discriminant;
};

//RWStructuredBuffer<Fragment> fragments  : register(u0);
// index of root node of each pixel
Texture2D<int> rootBuffer               : register(t0);
StructuredBuffer<int> firstBuffer       : register(t1);
// References to next fragment node index for each node in linked list
StructuredBuffer<float4> boundaryBuffer : register(t2);
StructuredBuffer<PITNode> nodeBuffer    : register(t3);
StructuredBuffer<int> nextBuffer        : register(t4);
StructuredBuffer<int> preorderBuffer    : register(t5);
StructuredBuffer<int> skipBuffer        : register(t6);
StructuredBuffer<int> depth             : register(t7);

StructuredBuffer<int> Morton            : register(t8);
StructuredBuffer<int> StartMipMaps      : register(t9);
StructuredBuffer<float2> MipMaps        : register(t10);

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

int get_index(int2 px, int view, int level)
{
    int res = Width >> level;
    return StartMipMaps[level] + Morton[px.x] + Morton[px.y] * 2 + view * res * res;
}

float4 main(float4 P: SV_POSITION, float2 C : TEXCOORD) : SV_TARGET
{
    float2 pNorm = C;
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

    int nodeCount = 0;
    int fragmentCount = 0;
    int currentNode = rootBuffer[coord];

    int2 px = coord >> 2;
    int view = px.x / (Width >> 2);
    int index = get_index(px % (Width >> 2), view, 5);

    return float4((MipMaps[index].y - MipMaps[index].x) * float3(0.00, 0.57, 0.59), 1);

    /*float maxDepth = boundaryBuffer[currentNode].w;
    float minDepth = boundaryBuffer[currentNode].z;*/

    //return float4((maxDepth - minDepth) * float3(0.00, 0.27, 0.29), 1);

    int maxNodeDepth = 0;

    int maxFragsInANode = 0;

    //for (int i=0; i<5 && currentNode != -1; i++)
    while (currentNode != -1)
    {
        nodeCount++;
        int currentFragment = firstBuffer[currentNode];

        int nodeFrags = 0;
        //for (int k=0; k<100 && currentFragment != -1; k++)
        while (currentFragment != -1)
        {
            nodeFrags++;
            fragmentCount++;

            currentFragment = nextBuffer[currentFragment];
        }

        maxFragsInANode = max(maxFragsInANode, nodeFrags);

        maxNodeDepth = max(maxNodeDepth, depth[currentNode]);

        currentNode = preorderBuffer[currentNode];
    }

    float ave = fragmentCount / (float)nodeCount;

    // Variance
    currentNode = rootBuffer[coord];
    float difference = 0;

    //for (int i=0; i<5 && currentNode != -1; i++)
    while (currentNode != -1)
    {
        int currentFragment = firstBuffer[currentNode];

        int nodeFrags = 0;
        //for (int k=0; k<100 && currentFragment != -1; k++)
        while (currentFragment != -1)
        {
            nodeFrags++;

            currentFragment = nextBuffer[currentFragment];
        }

        difference += abs(nodeFrags - ave);

        currentNode = preorderBuffer[currentNode];
    }

    if (nodeCount == 0)
        return float4(1, 0, 0.5, 1);

    //return float4(GetColor(maxNodeDepth / log(nodeCount) + 1), 1);
    return float4(GetColor(fragmentCount / nodeCount), 1);
    //return float4(GetColor(maxFragsInANode), 1);
    //return float4(GetColor(1 << (10 * (int) difference / fragmentCount)), 1);
    //return float4(GetColor(fragmentCount * log(nodeCount) / nodeCount), 1);
}