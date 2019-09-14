cbuffer ScreenInfo : register (b0) {
    int Width;
    int Height;
}

struct Fragment {
    int Index;
    float Min;
    float Max;
};

// Linked lists of fragments for each pixel
RWStructuredBuffer<Fragment> fragments  : register(u0);
// index of root node of each pixel
RWTexture2D<int> firstBuffer            : register(u1);
// References to next fragment node index for each node in linked list
RWStructuredBuffer<int> nextBuffer      : register(u2);
// buffer for fragment allocation
RWStructuredBuffer<int> malloc          : register(u3);

struct GS_OUT {
    float4 proj : SV_POSITION;
    float3 P0 : POSITION0;
    float3 P1 : POSITION1;
    float3 P2 : POSITION2;
    int TriangleIndex : TRIANGLEINDEX;
    int ViewIndex : VIEWINDEX;
};

void Intersect(float3 dir, float3 n, float d, inout float minim, inout float maxim) {
    float nDOTdir = dot(n, dir);

    if (nDOTdir == 0.00) {
        minim = 0;
        maxim = 10000;
        return;
    }
    float t = d / nDOTdir;

    /*if (t < 0)
    {
    minim = 0;
    maxim = 10000;
    return;
    }*/

    float depth = dir.z * t;

    minim = min(depth, minim);
    maxim = max(depth, maxim);
}

void main(GS_OUT input)
{
    uint2 pxy = input.proj.xy;
    // Get 2D coordinates of the a-buffer entry
    int triangleIndex = input.TriangleIndex;

    float2 dimensions = float2(Width, Height);
    // * Test real pixels

    int pW = Width;
    int pH = Height;

    // half pixel size in view coordinates
    float2 halfPixelSize = 1 / float2(pH, pH);

    float2 ndc = input.proj.xy / float2(Width, Height) * 2 - 1;
    ndc.y *= -1;
    // current pixel center in projection coordinates
    float3 p = float3(ndc, 1); // looking to pos Z

    // current pixel corners
    float3 p00 = p + float3(-halfPixelSize.x, halfPixelSize.y, 0);
    float3 p01 = p + float3(halfPixelSize.x, halfPixelSize.y, 0);
    float3 p10 = p + float3(-halfPixelSize.x, -halfPixelSize.y, 0);
    float3 p11 = p + float3(halfPixelSize.x, -halfPixelSize.y, 0);

    //float4x4 worldView = mul(Transforms[OB[triangleIndex * 3]], View);

    // current triangle vertexes transformed to current face coordinates
    float3 a = input.P0;//mul(float4(Vertexes[triangleIndex *3+0].P, 1), worldView).xyz;
    float3 b = input.P1;//mul(float4(Vertexes[triangleIndex *3+1].P, 1), worldView).xyz;
    float3 c = input.P2;//mul(float4(Vertexes[triangleIndex *3+2].P, 1), worldView).xyz;

    float3 abN = cross(a, b);
    float3 bcN = cross(b, c);
    float3 caN = cross(c, a);

    float sign = dot(c, abN) < 0 ? -1 : 1;

    /*if ((dot(abN, p00) < 0 && dot(abN, p01) < 0 && dot(abN, p11) < 0 && dot(abN, p10) < 0) ||
    (dot(bcN, p00) < 0 && dot(bcN, p01) < 0 && dot(bcN, p11) < 0 && dot(bcN, p10) < 0) ||
    (dot(caN, p00) < 0 && dot(caN, p01) < 0 && dot(caN, p11) < 0 && dot(caN, p10) < 0))
    return;*/

    int3x4 m = mul(
        float3x3(abN, bcN, caN) * sign,
        transpose(float4x3(p00, p01, p11, p10))) < 0;

    if (any(int3(all(m._m00_m01_m02_m03), all(m._m10_m11_m12_m13), all(m._m20_m21_m22_m23))))
        return;
    // * End test

    // Computing fragment interval [min,max]

    float minDepth = 1000000;
    float maxDepth = 0;

    float3 BA = b - a;
    float3 CA = c - a;
    float3 BAxCA = cross(BA, CA);

    float3 n = (BAxCA);
    float d = dot(n, a);

    // check pixel corners vs. triangle
    Intersect(p00, n, d, minDepth, maxDepth);
    Intersect(p01, n, d, minDepth, maxDepth);
    Intersect(p11, n, d, minDepth, maxDepth);
    Intersect(p10, n, d, minDepth, maxDepth);

    minDepth = max(minDepth, min(a.z, min(b.z, c.z)));
    maxDepth = min(maxDepth, max(a.z, max(b.z, c.z)));

    uint2 coord = pxy + uint2(Width * input.ViewIndex, 0);
    int index;
    InterlockedAdd(malloc[0], 1, index);
    InterlockedExchange(firstBuffer[coord], index, nextBuffer[index]);

    fragments[index].Index = triangleIndex;
    fragments[index].Min = minDepth;
    fragments[index].Max = maxDepth;
}