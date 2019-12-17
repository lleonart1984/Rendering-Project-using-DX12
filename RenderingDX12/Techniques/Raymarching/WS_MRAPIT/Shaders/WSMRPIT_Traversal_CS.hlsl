#include "../../../Common/CS_Constants.h"
#define PRECISION 1e7

// Data for each fragment
struct VertexData {
    float3 Position;
    float3 Normal;
    float2 Coordinates;
    float3 Tangent;
    float3 Binormal;
};

struct PITNode {
    int Parent;
    int Children[2];
    float Discriminant;
};

struct Fragment {
    int Index;
    float MinDepth;
    float MaxDepth;
};

struct RayInfo {
    float3 Position;
    float3 Direction;
    float3 AccumulationFactor;
};

struct RayIntersection {
    int TriangleIndex;
    float3 Coordinates;
};

struct MinMax {
    int3 min;
    int3 max;
};

// Ray information input
StructuredBuffer<RayInfo> rays                  : register (t0);
Texture2D<int> rayHeadBuffer                    : register (t1);
StructuredBuffer<int> rayNextBuffer             : register (t2);
// Scene Geometry
StructuredBuffer<VertexData> wsVertices         : register (t3);
StructuredBuffer<int3> sceneBoundaries          : register (t4);
// APIT
StructuredBuffer<Fragment> fragments            : register (t5);
StructuredBuffer<int> firstBuffer               : register (t6);
StructuredBuffer<float4> boundaryBuffer         : register (t7);
StructuredBuffer<int> rootBuffer                : register (t8); // first buffer updated to be tree root nodes...
StructuredBuffer<int> nextBuffer                : register (t9); // next buffer updated to be links of each per-node lists
StructuredBuffer<int> preorderBuffer            : register (t10);
StructuredBuffer<int> skipBuffer                : register (t11);
StructuredBuffer<MinMax> boundaries             : register (t12);

RWStructuredBuffer<RayIntersection> hits        : register (u0);
RWTexture2D<int> complexity                     : register (u1);

cbuffer ScreenInfo : register (b0)
{
    int Width;
    int Height;
    int Levels;
}

cbuffer RaymarchingInfo : register (b1)
{
    int CountSteps;
    int CountHits;
}

cbuffer ComputeShaderInfo : register(b2)
{
    uint3 InputSize;
}

int GetMRIndex(uint2 pxy, int level)
{
    /*
     * Let G(q, n) the Geometric Progression of N terms with ratio q
     * G(q, n) = (q^(n + 1) - 1) / (q - 1)
     * Sum from i = a to b: 2^i x 2^i = G(4, b) - G(4, a - 1)
     */
    int eMaxLevel = (Levels + 1) << 1;
    int eCurrentLevel = (Levels - level + 1) << 1;
    int skippedEntries = ((1 << eMaxLevel) - 1) / 3 - ((1 << eCurrentLevel) - 1) / 3; // G(4, Levels + 1) - G(4, Levels - level + 1)

    return skippedEntries + pxy.x + pxy.y * (1 << (Levels - level));
}

bool HitTest(float3 O, float3 D, float3 V1, float3 V2, float3 V3,
    inout float t, inout float3 coords)
{
    float3 e1, e2;
    float3 P, Q, T;
    float det, inv_det, u, v;

    e1 = V2 - V1;
    e2 = V3 - V1;

    P = cross(D, e2);
    det = dot(e1, P);

    if (det == 0) return false;

    inv_det = 1 / det;

    T = O - V1;

    Q = cross(T, e1);

    float3 uvt = float3 (dot(T, P), dot(D, Q), dot(e2, Q)) * inv_det;

    [branch]
    if (any(uvt < 0) || uvt.x + uvt.y > 1 || uvt.z >= t) return false;

    coords = float3(1 - uvt.x - uvt.y, uvt.x, uvt.y);
    t = uvt.z;

    return true;
}

void CheckIntersection(int index, float3 rayOrigin, float3 rayDirection, inout float minDepth, inout float maxDepth,
    inout float t, inout float3 coords, inout int rayHit)
{
    if (HitTest(rayOrigin, rayDirection,
        wsVertices[3 * index + 0].Position,
        wsVertices[3 * index + 1].Position,
        wsVertices[3 * index + 2].Position, t, coords)) {
        rayHit = index;
    }
}

void AnalizeNode(int node, inout float minDepth, inout float maxDepth, float3 rayOrigin, float3 rayDirection, inout float t, inout float3 coords, inout int rayHit, inout int counter)
{
    if (minDepth > boundaryBuffer[node].y || maxDepth < boundaryBuffer[node].x)
        return;

    int currentFragment = firstBuffer[node];
    while (currentFragment != -1)
    {
        counter += CountHits;

        Fragment f = fragments[currentFragment];
        CheckIntersection(f.Index, rayOrigin, rayDirection, minDepth, maxDepth, t, coords, rayHit);

        currentFragment = nextBuffer[currentFragment];
    }

    //counter += CountHits; // count current node
}

void QueryRange(uint2 px, int level, float minDepth, float maxDepth,
    float3 rayOrigin, float3 rayDirection, inout float t, inout float3 coords, inout int rayHit, inout int counter)
{
    int rootBufferIndex = GetMRIndex(px, level);
    int currentNode = rootBuffer[rootBufferIndex];

    while (currentNode != -1)
    {
        bool skipped = minDepth > boundaryBuffer[currentNode].w || maxDepth < boundaryBuffer[currentNode].z;
        if (!skipped) {
            AnalizeNode(currentNode, minDepth, maxDepth, rayOrigin, rayDirection, t, coords, rayHit, counter);
        }

        currentNode = skipped ? skipBuffer[currentNode] : preorderBuffer[currentNode];
    }
}

void Raymarch(int2 px, int rayIndex, float3 bMin, float3 bMax, inout int counter)
{
    RayInfo ray = rays[rayIndex];
    RayIntersection intersection = (RayIntersection)0;
    intersection.TriangleIndex = -1;
    intersection.Coordinates = float3(0, 0, 0);
    hits[rayIndex] = intersection;

    float3 P = ray.Position;
    float3 D = normalize(ray.Direction);
    if (dot(P, P) > 10000) // too far position
        return;

    bool cont;
    bool hit;

    float2x3 C = float2x3(bMin - P, bMax - P);
    float2x3 D2 = float2x3(D, D);
    float2x3 T = D2 == 0 ? float2x3(float3(-1000, -1000, -1000), float3(1000, 1000, 1000)) : C / D2;
    float tMin = max(max(min(T._m00, T._m10), min(T._m01, T._m11)), min(T._m02, T._m12));
    float tMax = min(min(max(T._m00, T._m10), max(T._m01, T._m11)), max(T._m02, T._m12));
    if (tMax < tMin || tMax < 0) {
        return;
    }

    tMin = max(0, tMin);

    float3 wsRayOrigin = P + D * tMin;
    float3 wsRayDirection = D * (tMax - tMin);

    // Ray positions in scene boundaries (Apit Cube)
    float3 a = (wsRayOrigin - bMin) / (bMax - bMin);
    float3 b = (wsRayOrigin + wsRayDirection - bMin) / (bMax - bMin);

    float t = 1.0;
	//for (int level = Levels; level >= 0; level--) {
	for (int level = 0; level <= Levels; level++) {
        // Screen positions of the ray
        int2 dimensions = int2(Width, Height) >> level;

        float2 screenIn = float2(a.x, 1 - a.y) * dimensions;
        float2 screenOut = float2(b.x, 1 - b.y) * dimensions;
        float2 screenDirection = screenOut - screenIn;

        int2 pixelBorder = (screenOut > screenIn) + int2(screenIn);
        float2 step = screenDirection == 0 ? 100 : 1 / abs(screenDirection);

        float2 alpha = screenDirection == 0 ? 1000 : (pixelBorder - screenIn) / screenDirection;
        int2 pixelInc = screenDirection > 0 ? 1 : -1;

        float currentAlpha = 0;
        float currentZ = a.z;
        int2 currentPixel = int2(screenIn);
        while (currentAlpha < t)
        {
            int2 selection = int2(alpha.x <= alpha.y, alpha.x > alpha.y);
            float nextAlpha = min(t, dot(selection, alpha));
            float nextZ = a.z + (b.z - a.z) * nextAlpha;

            QueryRange(currentPixel, level, min(currentZ, nextZ), max(currentZ, nextZ), wsRayOrigin, wsRayDirection, t,
                intersection.Coordinates, intersection.TriangleIndex, counter);

            alpha += selection * step;
            currentPixel += selection * pixelInc;

            currentAlpha = nextAlpha;
            currentZ = nextZ;

			counter += CountSteps;
        }
    }

    hits[rayIndex] = intersection;
}

[numthreads(CS_GROUPSIZE_2D, CS_GROUPSIZE_2D, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    /*float3 bMin = sceneBoundaries[0] / PRECISION;
    float3 bMax = sceneBoundaries[1] / PRECISION;*/

    float3 bMin = boundaries[0].min / PRECISION;
    float3 bMax = boundaries[0].max / PRECISION;

	int comp = 0;

    int2 px = DTid.xy;
    int rayIndex = rayHeadBuffer[px];
    while (rayIndex != -1)
    {
        Raymarch(px, rayIndex, bMin, bMax, comp);
        rayIndex = rayNextBuffer[rayIndex];
    }

	complexity[DTid.xy] = comp;
}