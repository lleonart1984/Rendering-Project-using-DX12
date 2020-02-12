#include "../CommonGI/Parameters.h"

#define g (0.875)
#define one_minus_g2 (1.0 - g * g)
#define one_plus_g2 (1.0 + g * g)
#define one_over_2g (0.5 / g)

#ifndef pi
#define pi 3.1415926535897932384626433832795
#endif

float EvalPhase(float3 D, float3 L) {
	float cosTheta = dot(D, L);
	return 0.25 / pi * (one_minus_g2) / pow(one_plus_g2 - 2 * g * cosTheta, 1.5);
}


struct PSInput
{
	float4 Proj : SV_POSITION;
	float3 P	: POSITION;
	float3 V	: VIEWER;
	int Index : PHOTON_INDEX;
};

struct Photon {
	float3 Position;
	float3 Direction;
	float3 Intensity;
};

cbuffer Transforms : register(b0) {
	row_major matrix AccProjMatrix;
	row_major matrix AccViewMatrix;
};


cbuffer Volume : register(b1) {
	int3 Dimensions;
	float Density;
	float Absortion;
}

StructuredBuffer<Photon> Photons : register(t0);
Texture3D<float> Accum : register (t1);
Texture3D<float> Volume	: register(t2);

sampler VolumeSampler : register(s0);

float3 TransformFromWorldToAccum(float3 wpos) {
	float4 p = mul(mul(float4(wpos, 1), AccViewMatrix), AccProjMatrix);
	return float3(0.5 * p.xy / max(0.01, p.w) + 0.5, p.z);
}

void GetVolumeBox(out float3 minim, out float3 maxim) {
	int3 Dimensions;
	Volume.GetDimensions(Dimensions.x, Dimensions.y, Dimensions.z);

	float maxC = max(Dimensions.x, max(Dimensions.y, Dimensions.z));
	minim = -Dimensions * 0.5 / maxC;
	maxim = -minim;
}

//float3 main(PSInput input) : SV_TARGET
//{
//	Photon p = Photons[input.Index];
//	
//	float radius = 0.01;
//	
//	float d = length(p.Position - input.P);
//
//	if (!any(p.Intensity))
//		return 0;
//
//	if (d >= radius)
//		return 0;
//
//	float3 bMin, bMax;
//	GetVolumeBox(bMin, bMax);
//
//	
//	//return float4(lerp(float3(1, 0, 0), float3(0, 0, 1), transmitanceToViewer)*0.01, 1);
//
//	float distance = 2 * sqrt(radius * radius - d * d);
//
//	float totalAcc = 0;
//
//	int samples = 10;
//	for (float d = -distance/2; d <= distance/2; d += distance / samples) {
//		float3 accSamplePos = TransformFromWorldToAccum(input.P + input.V * d);
//		float transmitanceToViewer = Accum.Sample(VolumeSampler, accSamplePos, 0, 0);
//		
//		float3 volSamplePos = (input.P + input.V * d - bMin) / (bMax - bMin);
//		float densityAtP = Volume.Sample(VolumeSampler, volSamplePos, 0, 0) * Density;
//
//		totalAcc += exp(-transmitanceToViewer * Density) * densityAtP;
//	}
//
//	float3 phase = EvalPhase(normalize(input.V), normalize(p.Direction));
//	return p.Intensity * (totalAcc / samples) * distance * phase / (pi * radius *  radius) / (PHOTON_DIMENSION * PHOTON_DIMENSION);
//}

float3 main(PSInput input) : SV_TARGET
{
	Photon p = Photons[input.Index];

	float radius = PHOTON_RADIUS;

	float d = length(p.Position - input.P);

	if (!any(p.Intensity) || isnan(p.Intensity.x))
		return 0;

	if (d >= radius)
		return 0;

	float3 bMin, bMax;
	GetVolumeBox(bMin, bMax);

	float3 accSamplePos = TransformFromWorldToAccum(p.Position);
	float transmitanceToViewer = Accum.Sample(VolumeSampler, accSamplePos, 0, 0);

	float3 volSamplePos = (input.P - bMin) / (bMax - bMin);
	float densityAtP = Volume.Sample(VolumeSampler, volSamplePos, 0, 0) * Density;

	float photonContrib = exp(-transmitanceToViewer * Density);

	float kernel = 1 - d / radius;

	float3 phase = EvalPhase(normalize(input.V), normalize(p.Direction));
	return p.Intensity * phase * photonContrib * kernel * 3 / (pi * radius * radius) / (PHOTON_DIMENSION * PHOTON_DIMENSION);
	//return p.Intensity * phase * photonContrib * kernel;
}