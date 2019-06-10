#ifndef SCATTERING_TOOLS_H
#define SCATTERING_TOOLS_H

#include "../Randoms/RandomHLSL.h"

// Prototype function will be implemented by
// global illumination algorithms using this header file.
// Resolves the visibility between point light source an a specific surfel.
float ShadowCast(Vertex surfel);

// Prototype function will be implemented by
// global illumination algorithms using this header.
// Resolves the point light sphere radius for path-traced rays.
float LightSphereRadius();

// Compute fresnel reflection component given the cosine of input direction and refraction index ratio.
// Refraction can be obtained subtracting to one.
// Uses the Schlick's approximation
float ComputeFresnel(float NdotL, float ratio)
{
	float oneMinusRatio = 1 - ratio;
	float onePlusRatio = 1 + ratio;
	float divOneMinusByOnePlus = oneMinusRatio / onePlusRatio;
	float f = divOneMinusByOnePlus * divOneMinusByOnePlus;
	return (f + (1.0 - f) * pow((1.0 - NdotL), 5));
}

// Transform every surfel component with a specific transform
// Valid for isometric transformation since directions are transformed
// using a simple multiplication operation.
Vertex Transform(Vertex surfel, float4x3 transform) {
	Vertex v = {
		mul(float4(surfel.P, 1), transform),
		mul(float4(surfel.N, 0), transform),
		surfel.C,
		mul(float4(surfel.T, 0), transform),
		mul(float4(surfel.B, 0), transform) };
	return v;
}

// Given a surfel will modify the normal with texture maps, using
// Bump mapping and masking textures.
// Material info is updated as well.
void AugmentHitInfoWithTextureMapping(inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0) : 1;

	float3x3 TangentToWorld = { surfel.T, surfel.B, surfel.N };
	// Change normal according to bump map
	surfel.N = normalize(mul(BumpTex * 2 - 1, TangentToWorld));

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

// Given a surfel will modify the material using texture mapping.
void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0).xyz : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0).xyz : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0).xyz : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

// Gets perfect lambertian normalized brdf ratio divided by a uniform distribution pdf value
float3 DiffuseBRDFMulTwoPi(float3 V, float3 L, float3 fN, float NdotL, Material material)
{
	return material.Diffuse * 2;
}

// Gets perfect lambertian normalized brdf ratio
float3 DiffuseBRDF(float3 V, float3 L, float3 fN, float NdotL, Material material) {
	return material.Diffuse / pi;
}

// Gets blinn-model specular normalized brdf ratio divided by a uniform distribution pdf value
float3 SpecularBRDFMulTwoPi(float3 V, float3 L, float3 fN, float NdotL, Material material)
{
	float3 H = normalize(V + L);
	float HdotN = max(0, dot(H, fN));
	return pow(HdotN, material.SpecularSharpness)*material.Specular*(2 + material.SpecularSharpness);
}

// Gets blinn-model specular normalized brdf ratio
float3 SpecularBRDF(float3 V, float3 L, float3 fN, float NdotL, Material material)
{
	float3 H = normalize(V + L);
	float HdotN = max(0, dot(H, fN));
	return pow(HdotN, material.SpecularSharpness)*material.Specular*(2 + material.SpecularSharpness) / two_pi;
}

// Gets if a specific direction hits the light taking in account
// the projected light area in a unit-hemisphere.
bool HitLight(float3 L, float3 D, float d, float LightRadius) {
	float loverd = LightRadius / d;
	float3 LMinusD = L - D;
	return dot(LMinusD, LMinusD) < loverd*loverd;
}

// Returns the radiance transmitted to the viewer direction 
// directly from a point light source
float3 ComputeDirectLightingNoShadowCast(
	// Inputs
	float3 V,
	Vertex surfel,
	Material material,
	float3 lightPosition,
	float3 lightIntensity,

	/// Outputs
	// absolute cosine of angle to viewer theta(w_in)
	out float NdotV,
	// gets if the surfel normal is inverted respect to viewer
	out bool invertNormal,
	// faced normal to the viewer
	out float3 fN,
	// reflection direction and factor
	out float4 R,
	// refraction direction and factor
	out float4 T
) {
	// compute cosine of angle to viewer respect to the surfel Normal
	NdotV = dot(V, surfel.N);
	// detect if normal is inverted
	invertNormal = NdotV < 0;
	NdotV = abs(NdotV); // absolute value of the cosine
	// Ratio between refraction indices depending of exiting or entering to the medium (assuming vaccum medium 1)
	float eta = invertNormal ? material.RefractionIndex : 1 / material.RefractionIndex;
	// Compute faced normal
	fN = invertNormal ? -surfel.N : surfel.N;
	// Gets the reflection component of fresnel
	R = float4(reflect(-V, fN), ComputeFresnel(NdotV, eta));
	// Compute the refraction component
	T = float4(refract(-V, fN, eta), 1 - R.w);

	if (!any(T.xyz))
	{
		R.w = 1;
		T.w = 0; // total internal reflection
	}
	// Mix reflection impulse with mirror materials
	R.w = material.Roulette.z + R.w * material.Roulette.w;
	T.w *= material.Roulette.w;

	// Vector to light (L)
	float3 L = lightPosition - surfel.P;
	// Sqr distance
	float d2 = dot(L, L);
	// Distance to light
	float d = sqrt(d2);
	// Normalize direction to light (L)
	L /= d;
	// compute lambert coefficient
	float NdotL = max(0, dot(fN, L));

	// Outgoing intensity per solid angle
	// Correct formula would be:
	// Total flux (light intensity) 
	// divided by: 4pi (Intensity per solid angle)
	// divided by 6 (the sphere is split in 6 cube faces)
	float3 I = lightIntensity / (4 * pi * 6);

	// Incomming irradiance per solid area
	// projected area converted to solid angle
	float3 Ip = I / (pi*d2);

	// Lambert Diffuse component (normalized dividing by pi)
	float3 DiffuseRatio = DiffuseBRDF(V, L, fN, NdotL, material);
	// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
	float3 SpecularRatio = SpecularBRDF(V, L, fN, NdotL, material);

	return 
		// Direct diffuse and glossy lighting
		(material.Roulette.x*DiffuseRatio + material.Roulette.y*SpecularRatio)*NdotL
		*Ip;
}

// Returns the radiance transmitted to the viewer direction 
// directly from a point light source
float3 ComputeDirectLighting(
	// Inputs
	float3 V,
	Vertex surfel,
	Material material,
	float3 lightPosition,
	float3 lightIntensity,
	float shadowFactor,

	/// Outputs
	// absolute cosine of angle to viewer theta(w_in)
	out float NdotV,
	// gets if the surfel normal is inverted respect to viewer
	out bool invertNormal,
	// faced normal to the viewer
	out float3 fN,
	// reflection direction and factor
	out float4 R,
	// refraction direction and factor
	out float4 T
) {
	// compute cosine of angle to viewer respect to the surfel Normal
	NdotV = dot(V, surfel.N);
	// detect if normal is inverted
	invertNormal = NdotV < 0;
	NdotV = abs(NdotV); // absolute value of the cosine
	// Ratio between refraction indices depending of exiting or entering to the medium (assuming vaccum medium 1)
	float eta = invertNormal ? material.RefractionIndex : 1 / material.RefractionIndex;
	// Compute faced normal
	fN = invertNormal ? -surfel.N : surfel.N;
	// Gets the reflection component of fresnel
	R = float4(reflect(-V, fN), ComputeFresnel(NdotV, eta));
	// Compute the refraction component
	T = float4(refract(-V, fN, eta), 1 - R.w);

	if (!any(T.xyz))
	{
		R.w = 1;
		T.w = 0; // total internal reflection
	}
	// Mix reflection impulse with mirror materials
	R.w = material.Roulette.z + R.w * material.Roulette.w;
	T.w *= material.Roulette.w;

	// Vector to light (L)
	float3 L = lightPosition - surfel.P;
	// Sqr distance
	float d2 = dot(L, L);
	// Distance to light
	float d = sqrt(d2);
	// Normalize direction to light (L)
	L /= d;
	// compute lambert coefficient
	float NdotL = max(0, dot(fN, L));

	// Outgoing intensity per solid angle
	// Correct formula would be:
	// Total flux (light intensity) 
	// divided by: 4pi (Intensity per solid angle)
	// divided by 6 (the sphere is split in 6 cube faces)
	float3 I = lightIntensity / (4 * pi * 6);

	// Incomming irradiance per solid area
	// projected area converted to solid angle
	float3 Ip = I / (pi*d2);

	// Lambert Diffuse component (normalized dividing by pi)
	float3 DiffuseRatio = DiffuseBRDF(V, L, fN, NdotL, material);
	// Blinn Specular component (normalized multiplying by (2+n)/(2pi)
	float3 SpecularRatio = SpecularBRDF(V, L, fN, NdotL, material);

	// gets the light radius constant
	float lightRadius = LightSphereRadius();

	// Direct Intensity gathered by reflection impulse
	float3 DirectReflection = material.Specular * HitLight(L, R.xyz, d, lightRadius);
	// Direct Intensity gathered by refraction impulse
	float3 DirectRefraction = material.Specular * HitLight(L, T.xyz, d, lightRadius);

	return shadowFactor * (
		// Direct diffuse and glossy lighting
		(material.Roulette.x*DiffuseRatio + material.Roulette.y*SpecularRatio)*NdotL*Ip
		+ (R.w * DirectReflection + T.w * DirectRefraction)*I / (4*pi*pi* lightRadius*lightRadius));
}


// Returns the radiance transmitted to the viewer direction 
// directly from a point light source
float3 ComputeDirectLighting(
	// Inputs
	float3 V,
	Vertex surfel,
	Material material,
	float3 lightPosition,
	float3 lightIntensity,

	/// Outputs
	// absolute cosine of angle to viewer theta(w_in)
	out float NdotV,
	// gets if the surfel normal is inverted respect to viewer
	out bool invertNormal,
	// faced normal to the viewer
	out float3 fN,
	// reflection direction and factor
	out float4 R,
	// refraction direction and factor
	out float4 T
) {
	return ComputeDirectLighting(
		V, surfel, material, lightPosition, lightIntensity,
		ShadowCast(surfel), NdotV, invertNormal, fN, R, T);
}

// Scatters a ray randomly using the material roulette information
// to decide between different illumination models (lambert, blinn, mirror, fresnel).
// the inverse of pdf value is already multiplied in ratio
// This method overload can use precomputed Reflectance (R) and Transmittance (T) vectors
void RandomScatterRay(float3 V, float3 fN, float4 R, float4 T, Material material,
	out float3 ratio,
	out float3 direction
	) {
	float NdotD;
	float3 D = randomHSDirection(fN, NdotD);

	float3 Diff = DiffuseBRDFMulTwoPi(V, D, fN, NdotD, material);
	float3 Spec = SpecularBRDFMulTwoPi(V, D, fN, NdotD, material);

	float4x3 ratios = {
		Diff * NdotD, // Diffuse
		Spec * NdotD, // Specular (Glossy)
		material.Specular, // Mirror
		material.Specular  // Fresnel
	};

	float4x3 directions = {
		D,
		D, // Could be improved using some glossy distribution
		R.xyz,
		T.xyz
	};

	float4 roulette = float4(material.Roulette.x, material.Roulette.y, R.w, T.w);

	float4 lowerBound = float4(0, roulette.x, roulette.x + roulette.y, roulette.x + roulette.y + roulette.z);
	float4 upperBound = lowerBound + roulette;

	float4 scatteringSelection = random();

	float4 selectionMask =
		(lowerBound <= scatteringSelection)
		* (scatteringSelection < upperBound);

	ratio = mul(selectionMask, ratios);
	direction = mul(selectionMask, directions);
}

// Scatters a ray randomly using the material roulette information
// to decide between different illumination models (lambert, blinn, mirror, fresnel).
// the inverse of pdf value is already multiplied in ratio
void RandomScatterRay(float3 V, Vertex surfel, Material material,
	out float3 ratio,
	out float3 direction
) {
	// compute cosine of angle to viewer respect to the surfel Normal
	float NdotV = dot(V, surfel.N);
	// detect if normal is inverted
	bool invertNormal = NdotV < 0;
	NdotV = abs(NdotV); // absolute value of the cosine
	// Ratio between refraction indices depending of exiting or entering to the medium (assuming vaccum medium 1)
	float eta = invertNormal ? material.RefractionIndex : 1 / material.RefractionIndex;
	// Compute faced normal
	float3 fN = invertNormal ? -surfel.N : surfel.N;
	// Gets the reflection component of fresnel
	float4 R = float4(reflect(-V, fN), ComputeFresnel(NdotV, eta));
	// Compute the refraction component
	float4 T = float4(refract(-V, fN, eta), 1 - R.w);

	if (!any(T.xyz))
	{
		R.w = 1;
		T.w = 0; // total internal reflection
	}
	// Mix reflection impulse with mirror materials
	R.w = material.Roulette.z + R.w * material.Roulette.w;
	T.w *= material.Roulette.w;

	RandomScatterRay(V, fN, R, T, material, ratio, direction);
}

void ComputeImpulses(float3 V, Vertex surfel, Material material,
	out float3 fN,
	out float4 R,
	out float4 T) {
	// compute cosine of angle to viewer respect to the surfel Normal
	float NdotV = dot(V, surfel.N);
	// detect if normal is inverted
	bool invertNormal = NdotV < 0;
	NdotV = abs(NdotV); // absolute value of the cosine
	// Ratio between refraction indices depending of exiting or entering to the medium (assuming vaccum medium 1)
	float eta = invertNormal ? material.RefractionIndex : 1 / material.RefractionIndex;
	// Compute faced normal
	fN = invertNormal ? -surfel.N : surfel.N;
	// Gets the reflection component of fresnel
	R = float4(reflect(-V, fN), ComputeFresnel(NdotV, eta));
	// Compute the refraction component
	T = float4(refract(-V, fN, eta), 1 - R.w);

	if (!any(T.xyz))
	{
		R.w = 1;
		T.w = 0; // total internal reflection
	}
	// Mix reflection impulse with mirror materials
	R.w = material.Roulette.z + R.w * material.Roulette.w;
	T.w *= material.Roulette.w;
}
#endif // !SCATTERING_TOOLS_H

