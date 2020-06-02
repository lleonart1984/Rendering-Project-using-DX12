
#include "../CommonGI/Definitions.h"
#include "../CommonGI/Parameters.h"
#include "../CommonGI/ScatteringTools.h"

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(t0);
Texture2D<float3> Normals				: register(t1);
Texture2D<float4> Coordinates			: register(t2);
Texture2D<int> MaterialIndices			: register(t3);
// Used for direct light visibility test
Texture2D<float3> LightPositions		: register(t4);

StructuredBuffer<Material> materials	: register(t5);
// Textures
Texture2D<float4> Textures[500]			: register(t6);

RWTexture2D<float3> DirectLighting		: register(u0);

// Used for texture mapping
SamplerState gSmp : register(s0);
// Used for shadow map mapping
SamplerState shadowSmp : register(s1);

cbuffer Lighting : register(b0) {
	float3 LightPosition;
	float3 LightIntensity;
}

// Global constant buffer with view to world transform matrix
cbuffer ViewToWorldTransform : register(b1) {
	row_major matrix ViewToWorld;
}

// Matrices to transform from world space to Light space (proj and view)
cbuffer LightTransforms : register(b2) {
	row_major matrix LightProj;
	row_major matrix LightView;
}

cbuffer ParticipatingMedia : register(b3) {
	float Extinction;
	float Phi;
}

// Gets true if current surfel is lit by the light source
// checking not with DXR but with classic shadow maps represented
// by GBuffer obtained from light
float ShadowCast(Vertex surfel)
{
	float3 pInLightViewSpace = mul(float4(surfel.P, 1), LightView).xyz;
	float4 pInLightProjSpace = mul(float4(pInLightViewSpace, 1), LightProj);
	if (pInLightProjSpace.z <= 0.01)
		return 0.0;
	pInLightProjSpace.xyz /= pInLightProjSpace.w;
	float2 cToTest = 0.5 + 0.5 * pInLightProjSpace.xy;
	cToTest.y = 1 - cToTest.y;
	float3 lightSampleP = LightPositions.SampleGrad(shadowSmp, cToTest, 0, 0);
	return pInLightViewSpace.z - lightSampleP.z < 0.001 ? 1 : 0.0;
}

// Given a surfel will modify the material using texture mapping.
void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material, float ddx, float ddy) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, ddx, ddy) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : material.Specular;
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, ddx, ddy).xyz : 1;

	material.Diffuse *= DiffTex.xyz * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

bool GetPrimaryIntersection(uint2 screenCoordinates, float2 coordinates, out float3 P, out float3 V,  out Vertex surfel, out Material material) {
	bool valid = any(Positions[screenCoordinates]);

	float4 C = Coordinates[screenCoordinates];

	surfel.P = Positions.SampleGrad(gSmp, coordinates, 0, 0);// [screenCoordinates];
	float d = length(surfel.P);

	surfel.N = Normals.SampleGrad(gSmp, coordinates, 0, 0);// [screenCoordinates];
	surfel.C = C.xy;//// .SampleGrad(gSmp, coordinates, 0.0001, 0.0001);// [screenCoordinates];
	surfel.T = float3(1, 0, 0);
	surfel.B = cross(surfel.N, surfel.T);

	V = -surfel.P / d;

	material = materials[MaterialIndices[screenCoordinates]];

	V = mul(float4(V, 0), ViewToWorld).xyz;
	P = mul(float4(0,0,0, 1), ViewToWorld).xyz;

	surfel = Transform(surfel, (float4x3)ViewToWorld);

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material, C.z, C.w);

	return valid;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float3 V;
	float3 P;
	Vertex surfel;
	Material material;
	int width, height;
	Positions.GetDimensions(width, height);

	float2 screenPos = (DTid.xy + 0.5) / float2(width, height);

	if (!GetPrimaryIntersection(DTid.xy, screenPos, P, V, surfel, material))
		return;

	float shadow = ShadowCast(surfel);

	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R;
	float4 T;

	float3 color = exp(-length(P - surfel.P) * Extinction) * (
		exp(-length(LightPosition - surfel.P)*Extinction) * ComputeDirectLighting(
		// Inputs
		V, surfel, material, LightPosition, LightIntensity,
		shadow,
		/// Outputs
		// absolute cosine of angle to viewer theta(w_in)
		NdotV,
		// gets if the surfel normal is inverted respect to viewer
		invertNormal,
		// faced normal to the viewer
		fN,
		// reflection direction and factor
		R,
		// refraction direction and factor
		T
	) + 
		material.Emissive);

	DirectLighting[DTid.xy] = color;
}