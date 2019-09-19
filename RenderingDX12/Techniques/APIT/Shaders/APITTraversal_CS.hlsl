#include "../../Common/CS_Constants.h"

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

struct RayInfo
{
    float3 Position;
    float3 Direction;
    float3 AccumulationFactor;
};

struct RayIntersection
{
    int TriangleIndex;
    float3 Coordinates;
};

RWStructuredBuffer<RayIntersection> hits : register(u0);
RWTexture2D<int> complexity : register (u1);

// Ray information input
StructuredBuffer<RayInfo> rays	                : register (t0);
Texture2D<int> rayHeadBuffer                    : register (t1);
StructuredBuffer<int> rayNextBuffer             : register (t2);
// Scene Geometry
StructuredBuffer<VertexData> triangles			: register (t3);
// APIT
StructuredBuffer<Fragment> fragments			: register (t4);
StructuredBuffer<int> firstBuffer				: register (t5);
StructuredBuffer<float4> boundaryBuffer			: register (t6);
Texture2D<int>			rootBuffer				: register (t7); // first buffer updated to be tree root nodes...
StructuredBuffer<int>	nextBuffer				: register (t8); // next buffer updated to be links of each per-node lists
StructuredBuffer<int>	preorderBuffer			: register (t9);
StructuredBuffer<int>	skipBuffer				: register (t10);
StructuredBuffer<float3x3> LayerTransforms		: register (t11);
// Empty-skipping strategy
StructuredBuffer<int> Morton                    : register (t12);
StructuredBuffer<int> StartMipMaps              : register (t13);
StructuredBuffer<float2> MipMaps                : register (t14);

cbuffer ScreenInfo : register (b0)
{
    int CubeLength;
    int Height;

    int pWidth;
    int pHeight;

    int Levels;
}

cbuffer RaymarchingInfo : register (b1) {
    int CountSteps;
    int CountHits;
}

int get_index(int2 px, int view, int level)
{
    int res = CubeLength >> level;
    return StartMipMaps[level] + Morton[px.x] + Morton[px.y] * 2 + view * res * res;
}

float2 Project(float3 P)
{
    return 0.5 * float2(P.x, -P.y) / P.z + 0.5;
}

float2 ToScreenPixel(float2 P, int faceIndex)
{
    return float2(P.x + faceIndex, P.y) * CubeLength;
}

bool IntersectSide(float3 O, float3 D, float3 V2, float3 V3,
    out float4 Pout)
{
    float3 P, Q;
    float det, inv_det, u, v;

    P = cross(D, V3);
    det = dot(V2, P);

    if (det == 0) return false;

    Q = cross(O, V2);

    float3 uvt = float3 (dot(O, P), dot(D, Q), dot(V3, Q)) / det;

    if (uvt.z < 0) return false;

    Pout = float4(O + D * uvt.z, uvt.z);
    return true;
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

    coords = float3 (1 - uvt.x - uvt.y, uvt.x, uvt.y);
    t = uvt.z;

    return true;
}

void CheckIntersection(int index, float3 rayOrigin, float3 rayDirection, inout float minDepth, inout float maxDepth,
    inout float t, inout float3 coords, inout int rayHit)
{
    if (HitTest(rayOrigin, rayDirection,
        triangles[3 * index + 0].Position,
        triangles[3 * index + 1].Position,
        triangles[3 * index + 2].Position, t,
        coords))
        rayHit = index;
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
        //if (f.MinDepth <= maxDepth && f.MaxDepth >= minDepth)
        CheckIntersection(f.Index, rayOrigin, rayDirection, minDepth, maxDepth,
            t, coords, rayHit);

        currentFragment = nextBuffer[currentFragment];
    }

    //counter += CountHits; // count current node
}


void QueryRange(uint2 px, float minDepth, float maxDepth,
    float3 rayOrigin, float3 rayDirection, inout float t, inout float3 coords, inout int rayHit, inout int counter)
{
    //int pxView = px.x / CubeLength;
    //int2 pxInView = px % CubeLength;
    //int index = get_index(pxInView, pxView, 0);

    //if (maxDepth < MipMaps[index].x || minDepth > MipMaps[index].y)
    //    return;

    int currentNode = rootBuffer[px];

    while (currentNode != -1)
    {
        bool skipped = minDepth > boundaryBuffer[currentNode].w || maxDepth < boundaryBuffer[currentNode].z;

        if (!skipped)
            AnalizeNode(currentNode, minDepth, maxDepth, rayOrigin, rayDirection, t, coords, rayHit, counter);

        currentNode = skipped ? skipBuffer[currentNode] : preorderBuffer[currentNode];
    }
}

void FixedAdvanceRayMarchingAtLevel(inout int counter, float3 rayOrigin, float3 rayDirection, float3 P, float3 H, int faceIndex,
    inout float t, inout float screent, inout int index, inout float3 rayHitCoords)
{
    float pixelSize = 1 / float(CubeLength);

    float2 projP = Project(P);
    float2 projH = Project(H);

    float2 npx = ToScreenPixel(projP, faceIndex);
    int2 px = int2(npx);

    float2 Ds = projH - projP;
    int2 sides = Ds >= 0;

    float2 dAlpha = pixelSize / Ds;
    float2 absDAlpha = abs(dAlpha);

    int2 inc = sign(Ds);// < 0 ? -1 : 1;
    float2 alpha = Ds == 0 ? 1 : (px + sides - npx) * dAlpha;

    float4 px_alpha = float4(px, alpha);
    float4 inc_absDAlpha = float4(inc, absDAlpha);

    float Pz = P.z;
    float invZ0 = 1 / P.z;
    float invZ1 = 1 / H.z;

    float selectedAlpha = 0;

    while (selectedAlpha < screent)
    {
        int2 sel = int2 (px_alpha.z <= px_alpha.w, px_alpha.z > px_alpha.w);

        selectedAlpha = min(1, dot(sel, px_alpha.zw));
        float newPz = 1 / lerp(invZ0, invZ1, selectedAlpha);

        QueryRange(px_alpha.xy, min(Pz, newPz), max(Pz, newPz), rayOrigin, rayDirection, t, rayHitCoords, index, counter);

        screent = min(1, H.z * t / lerp(P.z, H.z, t));
        Pz = newPz;

        px_alpha += inc_absDAlpha * sel.xyxy;

        counter += CountSteps;
    }
}

void FixedAdvanceRayMarchingAtFace(inout int counter, float3 P, float3 H, int faceIndex,
    inout int index, inout float3 rayHitCoords)
{
    float3 rayOrigin = P;
    float3 rayDirection = (H - P);

    float3x3 fromLocalToView = (LayerTransforms[faceIndex]);
    float3x3 fromViewToLocal = transpose(fromLocalToView);

    // normalize P and D.
    P = mul(P, fromViewToLocal);
    H = mul(H, fromViewToLocal);

    float t = 1;
    float screent = 1;

    FixedAdvanceRayMarchingAtLevel(counter, rayOrigin, rayDirection, P, H, faceIndex,
        t, screent, index, rayHitCoords);
}


float2 GetOccupiedRange(int2 px, int view, int level)
{
    return MipMaps[get_index(px, view, level)];
}

void AdaptiveAdvanceRayMarchingAtLevel(inout int counter, float3 rayOrigin, float3 rayDirection, float3 P, float3 H, int faceIndex,
    inout float t, inout float screent, inout int index, inout float3 rayHitCoords)
{

    int level = Levels - 1;

    int2 px = int2(0, 0);

    float2 projP = Project(P);
    float2 projH = Project(H);

    float2 Ds = projH - projP;
    int2 sides = Ds >= 0;

    float2 absDAlpha = abs(1 / Ds);

    int2 inc = sign(Ds); // < 0 ? -1 : 1;
    float2 alpha = Ds == 0 ? 1 : (sides - projP.xy) / Ds;

    float4 px_alpha = float4(px, alpha);
    float4 inc_absDAlpha = float4(inc, absDAlpha);

    float Pz = P.z;
    float invZ0 = 1 / P.z;
    float invZ1 = 1 / H.z;

    float2 CurrentP = projP;

    float selectedAlpha = 0;

    [loop]
    while (selectedAlpha < screent)
    {
        counter += CountSteps;

        int2 sel = int2(px_alpha.z <= px_alpha.w, px_alpha.z > px_alpha.w);

        float nextAlpha = min(1, dot(sel, px_alpha.zw));
        float newPz = 1 / lerp(invZ0, invZ1, nextAlpha);

        float2 interval = float2(min(newPz, Pz), max(newPz, Pz));
        float2 occupiedRange = GetOccupiedRange(px_alpha.xy, faceIndex, level);

        bool clearPath = interval.y < occupiedRange.x || interval.x > occupiedRange.y;

        [branch]
        if (!clearPath && level > 0)
        {
            float2 center = ((int2(px_alpha.xy) + 0.5) / (1 << Levels - 1 - level));
            int2 childPx = int2(px_alpha.xy) * 2 + (CurrentP > center);
            inc_absDAlpha.zw /= 2;
            px_alpha.zw -= inc_absDAlpha.zw * (childPx % 2 != sides);
            px_alpha.xy = childPx;
            level--;
            continue;
        }

        [branch]
        if (!clearPath)
        {
            QueryRange(px_alpha.xy + int2(CubeLength * faceIndex, 0), interval.x, interval.y, rayOrigin, rayDirection, t, rayHitCoords, index, counter);
            screent = min(1, H.z * t / lerp(P.z, H.z, t));
        }

        // advance
        int2 lastPx = px_alpha.xy;
        px_alpha += inc_absDAlpha * sel.xyxy;
        px = px_alpha.xy;
        Pz = newPz;
        CurrentP = lerp(projP, projH, nextAlpha);
        selectedAlpha = nextAlpha;

        while (clearPath && level < Levels - 1 && any(px >> 1 != lastPx >> 1))
        {
            int2 updateMask = sides != px % 2;
            px_alpha.zw += inc_absDAlpha.zw * updateMask;
            inc_absDAlpha.zw *= 2;
            px >>= 1;
            lastPx >>= 1;
            level++;
        }

        px_alpha.xy = px;
    }
}

void AdaptiveAdvanceRayMarchingAtFace(inout int counter, float3 P, float3 H, int faceIndex,
    inout int index, inout float3 rayHitCoords)
{
    float3 rayOrigin = P;
    float3 rayDirection = (H - P);

    float3x3 fromLocalToView = (LayerTransforms[faceIndex]);
    float3x3 fromViewToLocal = transpose(fromLocalToView);

    // normalize P and D.
    P = mul(P, fromViewToLocal);
    H = mul(H, fromViewToLocal);

    float t = 1;
    float screent = 1;

    FixedAdvanceRayMarchingAtLevel(counter, rayOrigin, rayDirection, P, H, faceIndex,
        t, screent, index, rayHitCoords);
}


int GetFace(float3 v)
{
    float3 absv = abs(v);
    float maxim = max(absv.x, max(absv.y, absv.z));
    if (maxim == absv.x)
        return v.x < 0;
    if (maxim == absv.y)
        return 2 + (v.y < 0);
    return 4 + (v.z < 0);
}

void Raymarch(int2 px, int index)
{
    RayInfo ray = rays[index];

    RayIntersection intersection = (RayIntersection)0;
    intersection.TriangleIndex = -1;
    intersection.Coordinates = float3(0, 0, 0);
    hits[index] = intersection;

    float3 dir = normalize(ray.Direction);

    float3 pos = ray.Position;

    float3 origin = pos;

    if (dot(pos, pos) > 10000) // too far position
        return;

    bool cont;
    bool hit;

    float4 fHits[8];
    int fHitsCount = 0;

    float4 fHit;

    fHits[fHitsCount++] = float4(pos, 0);

    if (IntersectSide(origin, dir, float3(-1, 1, 1), float3(1, 1, 1), fHit)) fHits[fHitsCount++] = fHit;
    if (IntersectSide(origin, dir, float3(1, 1, 1), float3(1, -1, 1), fHit)) fHits[fHitsCount++] = fHit;
    if (IntersectSide(origin, dir, float3(1, -1, 1), float3(-1, -1, 1), fHit)) fHits[fHitsCount++] = fHit;
    if (IntersectSide(origin, dir, float3(-1, -1, 1), float3(-1, 1, 1), fHit)) fHits[fHitsCount++] = fHit;
    if (IntersectSide(origin, dir, float3(-1, 1, 1), float3(-1, 1, -1), fHit)) fHits[fHitsCount++] = fHit;
    if (IntersectSide(origin, dir, float3(-1, -1, 1), float3(-1, -1, -1), fHit)) fHits[fHitsCount++] = fHit;

    fHits[fHitsCount++] = float4(
        origin + dir * 1000, // final position at infinity
        1000);

    int lastfHits = fHitsCount - 2;
    /// Sorting hits against views frustum's sides
    for (int i = 1; i < lastfHits; i++)
        for (int j = i + 1; j <= lastfHits; j++)
            if (fHits[j].w < fHits[i].w)
            {
                float4 temp = fHits[j];
                fHits[j] = fHits[i];
                fHits[i] = temp;
            }

    int counter = 0;

    for (int i = 0; i <= lastfHits; i++)
    {
        AdaptiveAdvanceRayMarchingAtFace(counter, fHits[i].xyz, fHits[i + 1].xyz, GetFace(lerp(fHits[i].xyz, fHits[i + 1].xyz, 0.5)),
            intersection.TriangleIndex, intersection.Coordinates);

        if (intersection.TriangleIndex != -1)
            break;
    }

    hits[index] = intersection;

    complexity[px] += counter;
}

[numthreads(CS_GROUPSIZE_2D, CS_GROUPSIZE_2D, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    int2 px = DTid.xy;
    int currentIndex = rayHeadBuffer[px];

    while (currentIndex != -1)
    {
        Raymarch(px, currentIndex);

        currentIndex = rayNextBuffer[currentIndex];
    }
}