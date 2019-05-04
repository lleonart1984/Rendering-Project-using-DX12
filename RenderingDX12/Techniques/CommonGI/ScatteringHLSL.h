#include "../CommonGI/Definitions.h"
#include "../Randoms/RandomHLSL.h"

struct ScatteredRay {
	float3 Direction;
	float3 Ratio;
	float pdf;
};

// Computes the Fresnel contribution for a given entry direction.
void ComputeFresnel(float3 I, float3 faceNormal, float ratio,
	out float reflection, out float refraction,
	out float3 reflectionDir, out float3 refractionDir)
{
	// Schlick's approximation
	float f = ((1.0 - ratio) * (1.0 - ratio)) / ((1.0 + ratio) * (1.0 + ratio));
	float Ratio = f + (1.0 - f) * pow((1.0 + dot(I, faceNormal)), 5);
	reflection = saturate(Ratio);
	refraction = 1 - reflection;

	reflectionDir = reflect(I, faceNormal);
	refractionDir = refract(I, faceNormal, ratio);
	if (!any(refractionDir))
	{
		reflection = 1;
		refraction = 0;
	}
}

// Transform all vertex data with a specific transform matrix.
// Correct tangent transforms should be different but this can work with isotropic scaling and rotations.
Vertex Transform(Vertex surfel, float4x3 transform) {
	Vertex v = {
		mul(float4(surfel.P, 1), transform),
		mul(float4(surfel.N, 0), transform),
		surfel.C,
		mul(float4(surfel.T, 0), transform),
		mul(float4(surfel.B, 0), transform) };
	return v;
}

// Adds to the surfel data the normal pertubation present in a bump map
// And augment material information with texture mapping.
// This function requires all textures to be present in a Textures named register
inline void AugmentHitInfoWithTextureMapping(bool onlyMaterial, inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0) : 1;

	if (!onlyMaterial) {
		float3x3 TangentToWorld = { surfel.T, surfel.B, surfel.N };
		// Change normal according to bump map
		surfel.N = normalize(mul(BumpTex * 2 - 1, TangentToWorld));
	}

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

// Computes the basic blinn-phong local lighting model. N dot L value and Faced surfel is received to not be computed twice 
// (taking in account forward direction is usually used in other parts of the code)
float3 BlinnBRDF(float3 V, float3 L, float NdotL, float3 fN, Material material) {
	float3 H = normalize(V + L);
	float3 diff = material.Diffuse / pi; // normalized diffuse contribution
	float3 spec = NdotL > 0 ? pow(max(0, dot(H, fN)), material.SpecularSharpness)*material.Specular * (2 + material.SpecularSharpness) / two_pi : 0; // normalized specular contribution

	return diff + spec;
}

// Computes a light scatter generating the outgoing direction, the transmission ratio and the pdf value for direction selection.
ScatteredRay RandomScatter(float probD, float probR, float probT,
	float3 diffuse, float3 reflection, float3 refraction,
	float3 D, float3 R, float3 T,
	float diffusePdf, float reflectionPdf, float refractionPdf)
{
	float3 roulette = float3(probD, probR, probT);
	roulette.y += roulette.x;
	roulette.z += roulette.y;
	float3 minimRoulette = float3(0, roulette.x, roulette.y);
	float selection = random();
	float3 rouletteMask = minimRoulette <= selection && selection < roulette;

	ScatteredRay sr;
	sr.Ratio = mul(rouletteMask, float3x3(diffuse, reflection, refraction));
	sr.Direction = mul(rouletteMask, float3x3(D, R, T));
	sr.pdf = dot(rouletteMask, float3(diffusePdf, reflectionPdf, refractionPdf));

	return sr;
}

// Computes the direct lighting for a sphere-like light in some position.
// Retrieves as well, the direction to the light, the dot product between light direction and surfel normal, and the light distance
float3 DirectIncomingRadiance(float3 TotalLightIntensity, float3 LightPosition, 
	Vertex surfel, float3 fN,
	out float3 L, out float NdotL, out float d) { 
	L = 
		LightPosition + // Center of the light sphere 
		- surfel.P; // surfel position
	
	d = length(L); // distance to light center
	
	L /= d; // normalize direction to light L.

	NdotL = dot(L, fN);

	if (NdotL < 0)
		return 0; // Light direction doesnt face the viewer

	// Total Light Intensity is all flux of the light source
	// TotalLightIntensity / 4pi is the emittance per area unit outgoing irradiance
	// TotalLightIntensity / 4pi / 2pi is the emitted radiance per area per solid angle
	// multiplied by 2pi to get total gathered radiance (Intensity back in one direction)
	// divided by d^2 to get the incoming radiance again.

	return
		// Intensity per solid angle (assuming light sources emmit in every direction the same ammount of energy)
		// Note that full sphere would be 4pi area, and each dA emit in all hemisphere (2pi solid angle)
		// but the projected area gets the energy of the half 2pi, so, can be canceled 4 pi * 2pi / 2pi == 4 pi
		TotalLightIntensity / (2*two_pi)
		/ (d * d); // Distance falloff (Light area is not considered since it affects both intensity per area and projected solid angle.
}

// Gets if a specific direction hits the light taking in account
// the projected light area in a unit-hemisphere.
bool HitLight(float3 L, float3 D, float d, float LightRadius) {
	float loverd = LightRadius / d;
	float3 LMinusD = L - D;
	return dot(LMinusD, LMinusD) < loverd*loverd;
}