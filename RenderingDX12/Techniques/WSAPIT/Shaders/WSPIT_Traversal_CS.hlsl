#include "../../Common/CS_Constants.h"
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

// Ray information input
StructuredBuffer<RayInfo> rays                  : register (t0);
Texture2D<int> rayHeadBuffer                    : register (t1);
StructuredBuffer<int> rayNextBuffer             : register (t2);
// Scene Geometry
StructuredBuffer<VertexData> wsVertices          : register (t3);
StructuredBuffer<int3> sceneBoundaries          : register (t4);
// APIT
StructuredBuffer<Fragment> fragments            : register (t5);
StructuredBuffer<int> firstBuffer               : register (t6);
StructuredBuffer<float4> boundaryBuffer         : register (t7);
Texture2D<int> rootBuffer                       : register (t8); // first buffer updated to be tree root nodes...
StructuredBuffer<int> nextBuffer                : register (t9); // next buffer updated to be links of each per-node lists
StructuredBuffer<int> preorderBuffer            : register (t10);
StructuredBuffer<int> skipBuffer                : register (t11);

RWStructuredBuffer<RayIntersection> hits        : register (u0);
RWTexture2D<int> complexity                     : register (u1);

cbuffer ScreenInfo : register (b0)
{
    int Width;
    int Height;
}

cbuffer RaymarchingInfo : register (b1)
{
    int CountSteps;
    int CountHits;
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


void QueryRange(uint2 px, float minDepth, float maxDepth,
    float3 rayOrigin, float3 rayDirection, inout float t, inout float3 coords, inout int rayHit, inout int counter)
{
    int currentNode = rootBuffer[px];

    while (currentNode != -1)
    {
        bool skipped = minDepth > boundaryBuffer[currentNode].w || maxDepth < boundaryBuffer[currentNode].z;
        if (!skipped) {
            AnalizeNode(currentNode, minDepth, maxDepth, rayOrigin, rayDirection, t, coords, rayHit, counter);
        }

        currentNode = skipped ? skipBuffer[currentNode] : preorderBuffer[currentNode];
    }
}

void Raymarch(int2 px, int rayIndex, float3 bMin, float3 bMax)
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
    int counter = 0;

    float3 corners[2] = { bMin - P, bMax - P };

    float tMin = all(bMin < P && P < bMax) ? 0 : 1000;
    float tMax = 0;
    for (int i = 0; i < 3; i++) // component
    {
        if (D[i] != 0) {
            for (int j = 0; j < 2; j++) // bounds
            {
                float t = corners[j][i] / D[i];
                float3 intersection = P + D * t;
                if (t > 0 && all(bMin < intersection && intersection < bMax)) {
                    tMin = min(tMin, t);
                    tMax = max(tMax, t);
                }
            }
        }
    }

    if (tMin == 1000) {
        return;
    }

    float3 wsRayOrigin = P + D * tMin;
    float3 wsRayDirection = D * (tMax - tMin);

    // Ray positions in scene boundaries (Apit Cube)
    float3 a = (wsRayOrigin - bMin) / (bMax - bMin);
    float3 b = (wsRayOrigin + wsRayDirection - bMin) / (bMax - bMin);

    // Screen positions of the ray
    float2 dimensions = float2(Width, Height);
    float2 pixelSize = float2(1, 1) / dimensions;

    float2 screenIn  = float2(a.x, 1 - a.y) / pixelSize;
    float2 screenOut = float2(b.x, 1 - b.y) / pixelSize;
    float2 screenDirection = screenOut - screenIn;

    int2 nextPixel = (screenOut > screenIn) + int2(screenIn);
    float2 Ds = normalize(screenDirection);
    float2 step = 0;
    if (screenDirection.x == 0)
    {
        step.x = 1000; // parallel to x axis
        step.y = 1.0 / length(screenDirection);
    }
    else
    {
        float slope = screenDirection.y / screenDirection.x;
        step.x = length(float2(1, slope)) / length(screenDirection);
        step.y = slope == 0 ? 1000 : (length(float2(1.0 / slope, 1.0)) / length(screenDirection));
    }

    float2 alpha = screenDirection == 0 ? 1000 : (nextPixel - screenIn) / screenDirection;
    int2 pixelInc = int2(screenDirection.x > 0 ? 1 : -1, screenDirection.y > 0 ? 1 : -1);
    
    float currentAlpha = 0;
    float currentZ = wsRayOrigin.z;
    uint2 currentPixel = uint2(screenIn);
    float t = 1.0;
    while (currentAlpha < t)
    {
        int2 selection = int2(alpha.x <= alpha.y, alpha.x > alpha.y);
        float nextAlpha = min(t, dot(selection, alpha));
        float nextZ = a.z + (b.z - a.z) * nextAlpha;

        QueryRange(currentPixel, min(currentZ, nextZ), max(currentZ, nextZ), wsRayOrigin, wsRayDirection, t, intersection.Coordinates, intersection.TriangleIndex, counter);
        
        alpha += selection * step;
        currentPixel += selection * pixelInc;

        currentAlpha = nextAlpha;
        currentZ = nextZ;
    }

    hits[rayIndex] = intersection;
}

[numthreads(CS_GROUPSIZE_2D, CS_GROUPSIZE_2D, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 bMin = sceneBoundaries[0] / PRECISION;
    float3 bMax = sceneBoundaries[1] / PRECISION;

    int2 px = DTid.xy;
    int rayIndex = rayHeadBuffer[px];
    while (rayIndex != -1)
    {
        Raymarch(px, rayIndex, bMin, bMax);
        rayIndex = rayNextBuffer[rayIndex];
    }
}