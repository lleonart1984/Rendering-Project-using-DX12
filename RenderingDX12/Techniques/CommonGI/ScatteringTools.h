#ifndef SCATTERING_TOOLS_H
#define SCATTERING_TOOLS_H

#include "../Randoms/RandomHLSL.h"

float ComputeFresnel(float NdotL, float ratio)
{
	float oneMinusRatio = 1 - ratio;
	float onePlusRatio = 1 + ratio;
	float divOneMinusByOnePlus = oneMinusRatio / onePlusRatio;
	float f = divOneMinusByOnePlus * divOneMinusByOnePlus;
	return (f + (1.0 - f) * pow((1.0 - NdotL), 5));
}

Vertex Transform(Vertex surfel, float4x3 transform) {
	Vertex v = {
		mul(float4(surfel.P, 1), transform),
		mul(float4(surfel.N, 0), transform),
		surfel.C,
		mul(float4(surfel.T, 0), transform),
		mul(float4(surfel.B, 0), transform) };
	return v;
}

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

void AugmentMaterialWithTextureMapping(inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0) : 1;

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

float3 BRDFxLambertDivPDF(float3 V, float3 L, float3 fN, float NdotL, Material material) {
	float3 H = normalize(V + L);
	float3 diff = 2*material.Diffuse;// max(0, NdotL)*material.Diffuse * 2; // normalized dividing by pi but then pdf is two_pi, thats why multiplied by 2
	float HdotN = max(0, dot(H, fN));
	float3 spec = pow(HdotN, material.SpecularSharpness)*material.Specular;// *(2 + material.SpecularSharpness) : 0;

	return (diff + spec)*NdotL;
}

float ShadowCast(Vertex surfel);

float LightSphereRadius();

// Gets if a specific direction hits the light taking in account
// the projected light area in a unit-hemisphere.
bool HitLight(float3 L, float3 D, float d, float LightRadius) {
	float loverd = LightRadius / d;
	float3 LMinusD = L - D;
	return dot(LMinusD, LMinusD) < loverd*loverd;
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

	// gets the light radius constant
	float lightRadius = LightSphereRadius();
	// compute cosine of angle to viewer respect to the surfel Normal
	NdotV = dot(V, surfel.N);
	// detect if normal is inverted
	invertNormal = NdotV < 0;
	NdotV = abs(NdotV); // absolute value of the cosine
	// Ratio between refraction indices depending of exiting or entering to the medium (assuming vaccum medium 1)
	float eta = invertNormal ? material.Roulette.w : 1 / material.Roulette.w;
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
	R.w = material.Roulette.y + R.w * material.Roulette.z;
	T.w *= material.Roulette.z;

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

	// Incomming radiance
	// Correct formula would be:
	// Total flux (light intensity) 
	// divided by: 4pi*r*r (outgoing irradiance, energy per unit area)
	// divided by: 2pi (outgoing radiance, energy per unit area per solid angle)
	// multiplied by: 2pi*r*r/d^2 (projected area: gathered radiance in a solid angle) 
	float3 I = lightIntensity / (4 * pi);
	float3 Ip = I / d2;

	// Visibility to the light
	float visibility = ShadowCast(surfel);
	
	// Lambert diffuse component (normalized dividing by pi)
	float3 diffuseAdd = material.Diffuse  / pi;
	// Blinn Specular component (normalized dividing by (2+n)/(2pi)
	float3 H = normalize(V + L);
	float NdotH = max(0, dot(H, fN));
	float3 spec = material.Specular*pow(NdotH, material.SpecularSharpness);
	float3 specularAdd = spec;// *(2 + material.SpecularSharpness) / two_pi;

	float3 reflectionAdd = R.w * HitLight(L, R.xyz, d, lightRadius);
	float3 refractionAdd = T.w * HitLight(L, T.xyz, d, lightRadius);

	return visibility * (
		NdotL*material.Roulette.x*(diffuseAdd + specularAdd)* Ip
		+ (reflectionAdd + refractionAdd)*I/(lightRadius*lightRadius));
}

#endif // !SCATTERING_TOOLS_H

