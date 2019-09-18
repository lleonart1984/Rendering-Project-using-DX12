struct VertexData
{
    float3 P;
    float3 N;
    float2 C;
    float3 T;
    float3 B;
};

struct Material
{
    float3 Diffuse;
    float RefractionIndex;
    float3 Specular;
    float SpecularSharpness;
    int DiffuseMap;
    int SpecularMap;
    int BumpMap;
    int MaskMap;
    float3 Emissive;
    float4 Roulette; // x-diffuse, y-mirror, z-fresnell, w-reflection index
};

cbuffer Lighting : register(b0)
{
    float3 LightPosition;

    float3 LightIntensity;
}

cbuffer ViewTransform : register(b1)
{
    row_major matrix Projection;
    row_major matrix InverseView;
}

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

RWStructuredBuffer<RayInfo> Output : register(u1);
RWTexture2D<int> OutputHeadBuffer : register(u2);
RWStructuredBuffer<int> OutputNextBuffer : register(u3);
RWStructuredBuffer<int> Malloc : register(u4);

// Incoming Ray informations
StructuredBuffer<RayInfo> Input : register(t0);
Texture2D<int> HeadBuffer : register(t1);
StructuredBuffer<int> NextBuffer : register(t2);

StructuredBuffer<RayIntersection> Intersections : register(t3);
// List of triangles's vertices of Scene Geometry
StructuredBuffer<VertexData> triangles : register(t4);
StructuredBuffer<int> OB : register(t5);
StructuredBuffer<int> MB : register(t6);
// Materials and Textures of the scene
StructuredBuffer<Material> materials : register(t7);
Texture2D<float4> Textures[32] : register(t8);

sampler Sampler : register(s0);

float4 Sampling(int index, float2 coord)
{
    if (index < 0 || index > 31) {
        return float4(1, 1, 1, 1);
    }

    return Textures[index].Sample(Sampler, coord);
}

void ComputeFresnel(float3 dir, float3 faceNormal, float ratio, out float reflection, out float refraction)
{
    float f = ((1.0 - ratio) * (1.0 - ratio)) / ((1.0 + ratio) * (1.0 + ratio));

    float Ratio = f + (1.0 - f) * pow((1.0 + dot(dir, faceNormal)), 5);

    reflection = min(1, Ratio);
    refraction = max(0, 1 - reflection);
}

static float pi = 3.1415926;

float3 AnalizeRay(int2 px, int index)
{
    RayInfo inp = Input[index];

    // Read the data
    float3 dir = inp.Direction;
    float3 pos = inp.Position;
    float3 fac = inp.AccumulationFactor;

    RayIntersection inter = Intersections[index];

    int hit = inter.TriangleIndex;
    float3 hitCoord = inter.Coordinates; // use baricentric coordinates

    if (hit < 0 || dot(pos, pos) > 10000) // vanishes
        //return skybox.Sample(skyBoxSampler, mul(dir, (float3x3) InverseView)) * fac;
        return 0;

    if (length(fac) < 0.005)
        return float3(0, 0, 0);

    VertexData V1 = triangles[hit * 3 + 0];
    VertexData V2 = triangles[hit * 3 + 1];
    VertexData V3 = triangles[hit * 3 + 2];

    float3 position = mul(hitCoord, float3x3(V1.P, V2.P, V3.P));
    float3 normal = normalize(mul(hitCoord, float3x3(V1.N, V2.N, V3.N)));
    float2 coordinates = mul(hitCoord, float3x2(V1.C, V2.C, V3.C));

    Material material = materials[MB[OB[hit * 3]]];

    float3 diffuse = material.Diffuse * Sampling(material.DiffuseMap, float2(coordinates.x, 1 - coordinates.y)).xyz;
    float3 specular = material.Specular;
    bool isOpaque = material.Roulette.w == 0;

    float3 observer = -1 * dir;
    bool isOutside = dot(observer, normal) > 0;
    float3 facedNormal = isOutside ? normal : -1 * normal;

    float3 L = LightPosition - position;
    float D = length(L);
    L /= D;
    float3 Lin = LightIntensity / (4 * 3.141596 * 3.14159 * 6 * max(0.1, D * D));

    float reflection = 1, refraction = 0;
    float3 reflectingDirection = normalize(2 * facedNormal * dot(facedNormal, observer) - observer);
    float3 refractingDirection = float3(0, 0, 0);
    if (!isOpaque) // Compute fresnell's
    {
        float ratio = isOutside ? 1 / material.RefractionIndex : material.RefractionIndex;

        ComputeFresnel(dir, facedNormal, ratio, reflection, refraction);
        refractingDirection = refract(dir, facedNormal, ratio);

        if (!any(refractingDirection)) // internal reflection
        {
            reflection = 1; // refraction;
            refraction = 0;
        }
    }

    // normalize fresnell ratios with roulette
    reflection = reflection * min(1, material.Roulette.z + material.Roulette.w);
    refraction = refraction * material.Roulette.w;

    float3 brdf = material.Roulette.x * diffuse + material.Roulette.y * specular * pow(saturate(dot(L, reflectingDirection)), max(1, material.SpecularSharpness)); // brdf for local lighting
    float3 localLighting = material.Emissive + brdf * saturate(dot(facedNormal, L)) * Lin; // +diffuse*0.3*material.Roulette.x;
    
    if (reflection > 0)
    {
        RayInfo bRay = inp;
        bRay.Position = position + facedNormal * 0.005;
        bRay.Direction = reflectingDirection;
        bRay.AccumulationFactor = fac * reflection;
        int newIndex;
        InterlockedAdd(Malloc[0], 1, newIndex);
        Output[newIndex] = bRay;
        OutputNextBuffer[newIndex] = OutputHeadBuffer[px];
        OutputHeadBuffer[px] = newIndex;
    }

    if (refraction > 0)
    {
        RayInfo bRay = inp;
        bRay.Position = position - facedNormal * 0.005;
        bRay.Direction = refractingDirection;
        bRay.AccumulationFactor = fac * refraction;
        int newIndex;
        InterlockedAdd(Malloc[0], 1, newIndex);
        Output[newIndex] = bRay;
        OutputNextBuffer[newIndex] = OutputHeadBuffer[px];
        OutputHeadBuffer[px] = newIndex;
    }

    return fac * localLighting;
}

float4 main(float4 proj : SV_POSITION) : SV_TARGET0
{
    float3 accumulations = float3(0, 0, 0);
    int2 px = proj.xy;

    int currentIndex = HeadBuffer[px];

    [loop]
    while (currentIndex != -1)
    {
        accumulations += AnalizeRay(px, currentIndex);
        currentIndex = NextBuffer[currentIndex];
    }

    return float4(accumulations, 1);
}