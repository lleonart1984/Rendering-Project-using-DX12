#include "../CommonGI/Definitions.h"
#include "../CommonGI/ScatteringTools.h"

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
StructuredBuffer<Vertex> triangles : register(t4);
StructuredBuffer<int> OB : register(t5);
StructuredBuffer<int> MB : register(t6);
// Materials and Textures of the scene
StructuredBuffer<Material> materials : register(t7);
Texture2D<float4> Textures[500] : register(t8);

sampler gSmp : register(s0);

float ShadowCast(Vertex surfel) {
	return 1; // no shadowcast
}

// Given a surfel will modify the normal with texture maps, using
// Bump mapping and masking textures.
// Material info is updated as well.
void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material, float ddx = 0, float ddy = 0) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, ddx, ddy) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

void YieldRay(uint2 px, float3 P, float3 D, float3 Importance) {
	RayInfo bRay = (RayInfo)0;
	bRay.Position = P;
	bRay.Direction = D;
	bRay.AccumulationFactor = Importance;

	int newIndex;
	InterlockedAdd(Malloc[0], 1, newIndex);
	Output[newIndex] = bRay;
	OutputNextBuffer[newIndex] = OutputHeadBuffer[px];
	OutputHeadBuffer[px] = newIndex;
}

float3 RaytracingScattering(uint2 px, float3 V, float3 importance, Vertex surfel, Material material)
{
	float3 total = float3(0, 0, 0);

	// Adding Emissive
	total += material.Emissive;

	// Adding direct lighting
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;

	total += ComputeDirectLighting(
			V,
			surfel,
			material,
			LightPosition,
			LightIntensity,
			/*Out*/ NdotV, invertNormal, fN, R, T);

	if (any(material.Specular))
	{
		if (R.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			YieldRay(px, 
				surfel.P + fN * 0.001,
				R.xyz,
				R.w * material.Specular * importance
			);
		}

		if (T.w > 0.01) {
			// Trace the ray.
			// Set the ray's extents.
			YieldRay(px,
				surfel.P - fN * 0.001,
				T.xyz,
				T.w * material.Specular * importance
			);
		}
	}

	return importance * total;
}


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
        return 0;

    Vertex V1 = triangles[hit * 3 + 0];
    Vertex V2 = triangles[hit * 3 + 1];
    Vertex V3 = triangles[hit * 3 + 2];

	Vertex surfel = (Vertex)0;

    surfel.P = mul(hitCoord, float3x3(V1.P, V2.P, V3.P));
    surfel.N = normalize(mul(hitCoord, float3x3(V1.N, V2.N, V3.N)));
    surfel.C = mul(hitCoord, float3x2(V1.C, V2.C, V3.C));
	surfel.B = normalize(mul(hitCoord, float3x3(V1.B, V2.B, V3.B)));
	surfel.T = normalize(mul(hitCoord, float3x3(V1.T, V2.T, V3.T)));

    Material material = materials[MB[OB[hit * 3]]];

	AugmentMaterialWithTextureMapping(surfel, material);

    float3 V = -dir;

	return RaytracingScattering(px, V, fac, surfel, material);
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