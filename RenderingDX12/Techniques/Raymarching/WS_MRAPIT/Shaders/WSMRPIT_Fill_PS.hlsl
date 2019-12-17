
struct Fragment {
    int Index;
    float Min;
    float Max;
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

RWStructuredBuffer<Fragment> fragments  : register(u0); // Linked lists of fragments for each pixel
RWStructuredBuffer<int> firstBuffer     : register(u1); // index of root node of each pixel
RWStructuredBuffer<int> nextBuffer      : register(u2); // References to next fragment node index for each node in linked list
RWStructuredBuffer<int> malloc          : register(u3); // buffer for fragment allocation

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

void Intersect(float2 c, float3 P, float3 N, inout float minim, inout float maxim)
{
    if (N.z == 0.00) {
        minim = 0;
        maxim = 10000;
        return;
    }

    //float z = P.z + dot(P.xy - c, N.xy) / N.z;
    float z = dot(P - float3(c, 0), N) / N.z;
    minim = min(z, minim);
    maxim = max(z, maxim);
}

void main(GS_OUT input)
{
    uint2 pxy = input.proj.xy;
    int triangleIndex = input.TriangleIndex;
    int level = input.Level;

    // * Test real pixels

    // half pixel size in view coordinates
    float2 dimensions = float2(Width, Height);
    float2 halfPixelSize = (1 << level) / dimensions;

    // Converting to range [-1, 1]
    float2 ndc = input.proj.xy * halfPixelSize * 2 - 1;
    ndc.y *= -1;


    // current pixel corners
    float2 c00 = ndc + float2(-halfPixelSize.x, halfPixelSize.y);
    float2 c01 = ndc + float2(halfPixelSize.x, halfPixelSize.y);
    float2 c10 = ndc + float2(-halfPixelSize.x, -halfPixelSize.y);
    float2 c11 = ndc + float2(halfPixelSize.x, -halfPixelSize.y);

    // current triangle vertexes transformed to current face coordinates
    float3 a = input.P0;
    float3 b = input.P1;
    float3 c = input.P2;

    float2 abN = float2(b.y - a.y, a.x - b.x);
    float2 bcN = float2(c.y - b.y, b.x - c.x);
    float2 caN = float2(a.y - c.y, c.x - a.x);
    float sign = dot(c - a, abN) > 0 ? 1 : -1;

    abN *= sign;
    bcN *= sign;
    caN *= sign;

    if ((dot(abN, c00 - a) < 0 && dot(abN, c01 - a) < 0 && dot(abN, c11 - a) < 0 && dot(abN, c10 - a) < 0) ||
        (dot(bcN, c00 - b) < 0 && dot(bcN, c01 - b) < 0 && dot(bcN, c11 - b) < 0 && dot(bcN, c10 - b) < 0) ||
        (dot(caN, c00 - c) < 0 && dot(caN, c01 - c) < 0 && dot(caN, c11 - c) < 0 && dot(caN, c10 - c) < 0)) {
        return;
    }

    /*if (all(float4(dot(abN, c00), dot(abN, c01), dot(abN, c11), dot(abN, c10)) - dot(abN, a) < 0) ||
    all(float4(dot(bcN, c00), dot(bcN, c01), dot(bcN, c11), dot(bcN, c10)) - dot(bcN, b) < 0) ||
    all(float4(dot(caN, c00), dot(caN, c01), dot(caN, c11), dot(caN, c10)) - dot(caN, c) < 0))
        return;*/

        /*int3x4 m = mul(
            float3x3(float3(abN, dot(abN, a)), float3(bcN, dot(bcN, b)), float3(caN, dot(caN, c))) * sign,
            float3x4(transpose(float4x2(c00, c01, c11, c10)), -float4(1, 1, 1, 1))) < 0;

        if (any(int3(all(m._m00_m01_m02_m03), all(m._m10_m11_m12_m13), all(m._m20_m21_m22_m23))))
            return;*/
            // * End test

    float minDepth = 1000000;
    float maxDepth = 0;

    float3 BA = b - a;
    float3 CA = c - a;
    float3 n = cross(BA, CA);

    // check pixel corners vs. triangle
    Intersect(c00, a, n, minDepth, maxDepth);
    Intersect(c01, a, n, minDepth, maxDepth);
    Intersect(c11, a, n, minDepth, maxDepth);
    Intersect(c10, a, n, minDepth, maxDepth);

    minDepth = max(minDepth, min(a.z, min(b.z, c.z)));
    maxDepth = min(maxDepth, max(a.z, max(b.z, c.z)));
    
    int firstBufferIndex = GetMRIndex(pxy, level);

    int index;
    InterlockedAdd(malloc[0], 1, index);
    InterlockedExchange(firstBuffer[firstBufferIndex], index, nextBuffer[index]);

    fragments[index].Index = triangleIndex;
    fragments[index].Min = minDepth;
    fragments[index].Max = maxDepth;
}