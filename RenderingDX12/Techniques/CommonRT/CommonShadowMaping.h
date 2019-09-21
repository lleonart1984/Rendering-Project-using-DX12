/// Common shader header for shadow map computation support in a library
/// Requires:
/// A Texture2D of float3 with positions from light at LIGHTVIEW_POSITIONS_REG register
/// A constant buffer with proj and view matrices of light frustum at LIGHT_TRANSFORMS_CB_REG register
/// A sampler for shadow map accessing at SHADOW_MAP_SAMPLER_REG register
/// Provides a function to shadow cast using shadow map.

// Used for direct light visibility test
Texture2D<float3> LightPositions		: register(LIGHTVIEW_POSITIONS_REG);

// Matrices to transform from Light space (proj and view) to world space
cbuffer LightTransforms : register(LIGHT_TRANSFORMS_CB_REG) {
	row_major matrix LightProj;
	row_major matrix LightView;
}

// Used for shadow map mapping
SamplerState shadowSmp : register(SHADOW_MAP_SAMPLER_REG);

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
	return pInLightViewSpace.z - lightSampleP.z < 0.001 ? 1 : 0.8;
}