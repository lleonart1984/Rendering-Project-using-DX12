#include "../CommonGI/Parameters.h"

struct GSOutput
{
	float4 Proj : SV_POSITION;
	float3 P	: POSITION;
	float3 V	: VIEWER;
	int Index	: PHOTON_INDEX;
};

struct Photon {
	float3 Position;
	float3 Direction;
	float3 Intensity;
};

StructuredBuffer<Photon> Photons : register(t0);

cbuffer Camera : register(b0) {
	row_major matrix Projection;
	row_major matrix View;
};

[maxvertexcount(4)]
void main(
	point uint input[1] : PHOTON_INDEX,
	inout TriangleStream< GSOutput > output
)
{
	float3 V = normalize(View._m02_m12_m22);

	float3 center = Photons[input[0]].Position;
	float3 Pivot = float3(0, 1.012312, 0);
	float3 R = normalize(cross(V, Pivot));
	float3 U = normalize(cross(V, R));

	float radius = PHOTON_RADIUS;

	float3 C00 = center + R * radius + U * radius;
	float3 C01 = center + R * radius - U * radius;
	float3 C10 = center - R * radius + U * radius;
	float3 C11 = center - R * radius - U * radius;

	float4x4 viewProj = mul(View, Projection);

	GSOutput element = (GSOutput)0;
	element.Index = input[0];
	element.V = V;

	element.Proj = mul(float4(C00, 1), viewProj);
	element.P = C00;
	output.Append(element);

	element.Proj = mul(float4(C01, 1), viewProj);
	element.P = C01;
	output.Append(element);

	element.Proj = mul(float4(C10, 1), viewProj);
	element.P = C10;
	output.Append(element);

	element.Proj = mul(float4(C11, 1), viewProj);
	element.P = C11;
	output.Append(element);
}