#include "../CommonGI/Parameters.h"

#define pi 3.1415926535897932384626433832795

struct GSOutput
{
	float4 Proj : SV_POSITION;
	float3 X	: POSITION;
	float3 P	: PHOTON_POSITION;
	float3 I	: PHOTON_INTENSITY;
	float  K	: SPLAT_KERNEL;
};

struct Photon {
	float3 Position;
	float3 Contribution;
};

StructuredBuffer<Photon> Photons : register(t0);
Texture3D<float> VolumeData : register(t1);

sampler VolumeSampler	: register(s0);

cbuffer Camera : register(b0) {
	row_major matrix Projection;
	row_major matrix View;
};

[maxvertexcount(8)]
void main(
	point uint input[1] : PHOTON_INDEX,
	inout TriangleStream< GSOutput > output
)
{
	float3 V = normalize(View._m02_m12_m22);

	Photon p = Photons[input[0]];

	if (!any(p.Contribution) || isnan(p.Contribution.x))
		return; // colapse black photons

	float3 center = p.Position;
	float3 Pivot = float3(0, 1.012312, 0);
	float3 R = normalize(cross(V, Pivot));
	float3 U = normalize(cross(V, R));

	float radius = PHOTON_RADIUS;

	float3 bMin, bMax;
	int3 Dimensions;
	VolumeData.GetDimensions(Dimensions.x, Dimensions.y, Dimensions.z);

	bMin = -Dimensions * 0.5 / (float)max(Dimensions.x, max(Dimensions.y, Dimensions.z));
	bMax = -bMin;

	float3 C00 = center + R * radius + U * radius;
	float3 C01 = center + R * radius - U * radius;
	float3 C10 = center - R * radius + U * radius;
	float3 C11 = center - R * radius - U * radius;
	float3 M = center;

	float dM = VolumeData.SampleGrad(VolumeSampler, (M - bMin) / (bMax - bMin), 0, 0);
	float d00 = dM;// VolumeData.SampleGrad(VolumeSampler, (C00 - bMin) / (bMax - bMin), 0, 0);
	float d01 = dM;// VolumeData.SampleGrad(VolumeSampler, (C01 - bMin) / (bMax - bMin), 0, 0);
	float d10 = dM;// VolumeData.SampleGrad(VolumeSampler, (C10 - bMin) / (bMax - bMin), 0, 0);
	float d11 = dM;// VolumeData.SampleGrad(VolumeSampler, (C11 - bMin) / (bMax - bMin), 0, 0);

	float ave = max (0.0000001, d00 + d01 + d10 + d11 + dM) / 5;

	float4x4 viewProj = mul(View, Projection);

	GSOutput element = (GSOutput)0;
	element.P = p.Position;
	element.I = p.Contribution / (pi * radius * radius) / (PHOTON_DIMENSION * PHOTON_DIMENSION);

	GSOutput e00 = element;
	e00.Proj = mul(float4(C00, 1), viewProj);
	e00.X = C00;
	e00.K = d00 / ave;

	GSOutput e01 = element;
	e01.Proj = mul(float4(C01, 1), viewProj);
	e01.X = C01;
	e01.K = d01 / ave;

	GSOutput e10 = element;
	e10.Proj = mul(float4(C10, 1), viewProj);
	e10.X = C10;
	e10.K = d10 / ave;

	GSOutput e11 = element;
	e11.Proj = mul(float4(C11, 1), viewProj);
	e11.X = C11;
	e11.K = d11 / ave;

	GSOutput eM = element;
	eM.Proj = mul(float4(M, 1), viewProj);
	eM.X = M;
	eM.K = dM / ave;

	output.Append(e00);
	output.Append(e01);
	output.Append(eM);
	output.Append(e11);
	output.Append(e10);
	output.RestartStrip();
	output.Append(e10);
	output.Append(e00);
	output.Append(eM);
	output.RestartStrip();
}