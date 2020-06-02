/// Implementation of an iterative and accumulative path-tracing.
/// Primary rays and shadow cast use GBuffer optimization
/// Path-tracing with next-event estimation.

#include "../CommonGI/Definitions.h"

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

#define VERTICES_REG				t1
#define MATERIALS_REG				t2
#define GBUFFER_POSITIONS_REG		t3
#define GBUFFER_NORMALS_REG			t4
#define GBUFFER_COORDINATES_REG		t5
#define GBUFFER_MATERIALS_REG		t6
#define LIGHTVIEW_POSITIONS_REG		t7
#define BACKGROUND_IMG_REG			t8
#define TEXTURES_REG				t10

#define TEXTURES_SAMPLER_REG		s0
#define SHADOW_MAP_SAMPLER_REG		s1

#define OUTPUT_IMAGE_REG			u0
#define ACCUM_IMAGE_REG				u1

#define VIEWTOWORLD_CB_REG			b0
#define LIGHTING_CB_REG				b1
#define LIGHT_TRANSFORMS_CB_REG		b2
#define ACCUMULATIVE_CB_REG			b3
#define OBJECT_CB_REG				b8

#include "../CommonRT/CommonDeferred.h"
#include "../CommonRT/CommonShadowMaping.h"
#include "../CommonRT/CommonProgressive.h"
#include "../CommonRT/CommonOutput.h"
#include "../CommonGI/ScatteringTools.h"

struct Ray {
	float3 Position;
	float3 Direction;
};

struct RayPayload
{
	float3 Importance;
	float3 AccRadiance;
	Ray ScatteredRay;
	int bounces;
	int volBounces;
	ULONG seed;
};

StructuredBuffer<uint> Grid : register(t9);

cbuffer ParticipatingMedia : register(b4) {
	float3 Extinction;
	float3 G_Value;
	float3 Phi;
	float PathtracingPercentage;
}

cbuffer Transforming : register(b5) {
	row_major matrix FromProjectionToWorld;
}

cbuffer GridInfo : register(b6) {
	int Size;
	float3 MinimumGrid;
	float3 MaximumGrid;
}

cbuffer DebugInfo : register(b7) {
	int CountSteps;
}

#define VOL_DIST 4

uint GetValueAt(int3 cell) {
	int index = Size * (cell.y + cell.z * Size) + cell.x;
	return (Grid[index >> 3] >> ((index & 0x7) * 4)) & 0xF;
}

float MaximalRadius(float3 P) {

	//if (all(P > MinimumGrid) && all(P < MaximumGrid))
	{
		float3 cellSize = (MaximumGrid - MinimumGrid) / Size;
		int3 cell = (P - MinimumGrid) / cellSize;
		uint emptySizePower = GetValueAt(cell);

		if (emptySizePower <= 0) // no empty cell
			return 0;
		
		float halfboxSize = pow(2, emptySizePower - 1) - 0.5;// *0.5;
		float3 minBox = (cell + 0.5 - halfboxSize) * cellSize + MinimumGrid;
		float3 maxBox = (cell + 0.5 + halfboxSize) * cellSize + MinimumGrid;

		float3 toMin = (P - minBox);
		float3 toMax = (maxBox - P);
		float3 m = min(toMin, toMax);
		return min(m.x, min(m.y, m.z));
	}

	//float3 proj = P <= MinimumGrid ? MinimumGrid : (P >= MaximumGrid ? MaximumGrid : P);
	//return max(0, min(length(P - proj), VOL_DIST - length(P))); // distance to box and to the exterior of vol
}

void TransformWavelengthMaterial(inout Material material, float3 color) {
	material.RefractionIndex = pow(material.RefractionIndex, color.x > 0 ? 1 : color.y > 0 ? 0.92 : 0.82);
}

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void SurfelScattering(float3 V, Vertex surfel, Material material, inout RayPayload payload)
{
	TransformWavelengthMaterial(material, payload.Importance);

	// Adding emissive and direct lighting
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;
	// Update Accumulated Radiance to the viewer
	//payload.AccRadiance += payload.Importance *
	//	(material.Emissive
	//		// Next Event estimation
	//		+ ComputeDirectLighting(V, surfel, material, LightPosition, LightIntensity,
	//			// Co-lateral outputs
	//			NdotV, invertNormal, fN, R, T));

	ComputeImpulses(V, surfel, material,
		NdotV,
		invertNormal,
		fN,
		R,
		T);

	float3 ratio;
	float3 direction;
	float pdf;
	RandomScatterRay(V, fN, R, T, material, ratio, direction, pdf);

	// Update gathered Importance to the viewer
	payload.Importance *= max(0, ratio);// / (1 - russianRoulette);
	// Update scattered ray
	payload.ScatteredRay.Direction = direction;
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN)) * 0.00001 * fN;
}

#include "VolumeScattering.h"

float exitSphere(float3 p, float3 d)
{
	float3 D = d;
	float3 F = p;
	float a = dot(D, D);
	float b = 2 * dot(F, D);
	float c = dot(F, F) - 1;

	float Disc = b * b - 4 * a * c;

	if (Disc < 0)
		return 0;

	return max(0, (-b + sqrt(Disc)) / (2 * a));
}

void GenerateVariables(float G, float Phi, float3 win, float3 Lin, float density, out float3 x, out float3 w, out float3 X, out float Importance, out float Accum) {
	float prob = 1 - exp(-density);

	Importance = Phi;
	Accum = Phi * EvalPhase(G, win, Lin);

	x = 0;
	X = x;
	w = win;

	w = GeneratePhase(G, w);

	if (random() >= prob) // single scattering
	{
		x = w;
		return;
	}

	float t = abs(density) <= 0.001 ? 100000 : -log(max(0.000001, 1 - random() * (1 - exp(-density)))) / density;

	x = x + w * t;
	[loop]
	while (true)
	{
		Importance *= Phi;
		float currentPdf = EvalPhase(G, w, Lin) * Importance;
		Accum += currentPdf;
		if (random() < currentPdf / Accum)
			X = x;

		w = GeneratePhase(G, w);

		float d = exitSphere(x, w);

		t = abs(density) <= 0.001 ? 100000 : -log(max(0.00000001, 1 - random())) / density;

		[branch]
		if (t > d)
		{
			x += d * w;
			return;
		}

		x += t * w;
	}
}

#include "../VolumeRendering/SphereScatteringWithDL.h"

void GenerateVariablesWithModel(float G, float Phi, float3 win, float density, out float3 x, out float3 w, out float3 X, out float3 W, out float accum, out float absorption)
{
	x = float3(0, 0, 0);
	w = win;

	X = x;
	W = w;

	float prob = 1 - exp(-density);
	if (random() >= prob) // single scattering
	{
		w = GeneratePhase(G, win);
		x = w;
		accum = Phi;
		absorption = 1 - Phi;
		return;
	}

	float3 temp = abs(win.x) >= 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 winY = normalize(cross(temp, win));
	float3 winX = cross(win, winY);
	float rAlpha = random() * 2 * pi;
	float3x3 R = (mul(float3x3(
		cos(rAlpha), -sin(rAlpha), 0,
		sin(rAlpha), cos(rAlpha), 0,
		0, 0, 1), float3x3(winX, winY, win)));

	// Generate path
	float pathInput[12];
	float pathOutput[4];
	pathInput[0] = density;
	pathInput[1] = Phi;
	pathInput[2] = G;
	pathInput[3] = gauss();
	pathInput[4] = gauss();
	pathInput[5] = gauss();
	pathInput[6] = gauss();
	pathInput[7] = gauss();
	pathInput[8] = gauss();
	pathInput[9] = gauss();
	pathInput[10] = gauss();
	pathInput[11] = gauss();
	PathGen(pathInput, pathOutput);
	float4 pO = float4(pathOutput[0], pathOutput[1], pathOutput[2], pathOutput[3]);
	pO = clamp(pO * 2, -1, 1);
	float costheta = pO.x;
	float wt = pO.y;
	float dwt = sqrt(1 - wt * wt);
	float wb = clamp(pO.z * sign(random() - 0.5), -dwt, dwt);
	x = float3(0, sqrt(max(0, 1 - costheta * costheta)), costheta);
	float3 N = x;
	float3 B = float3(1, 0, 0);
	float3 T = cross(x, B);
	w = normalize(N * sqrt(max(0, 1 - wt * wt - wb * wb)) + T * wt + B * wb);
	float n = 2.0 / max(0.000001, 1 - pO.w);
	absorption = Phi == 1 ? 0 : 1 - pow(Phi, n);
	accum = Phi == 1 ? n : Phi * absorption / (1 - Phi);

	//float scatInput[16];
	//float scatOutput[6];
	//scatInput[0] = density;
	//scatInput[1] = Phi;
	//scatInput[2] = G;
	//scatInput[3] = pathOutput[0];
	//scatInput[4] = pathOutput[1];
	//scatInput[5] = pathOutput[2];
	//scatInput[6] = pathOutput[3];
	//scatInput[7] = gauss();
	//scatInput[8] = gauss();
	//scatInput[9] = gauss();
	//scatInput[10] = gauss();
	//scatInput[11] = gauss();
	//scatInput[12] = gauss();
	//scatInput[13] = gauss();
	//scatInput[14] = gauss();
	//scatInput[15] = gauss();
	////ScatGen(scatInput, scatOutput);
	//X = clamp(float3(scatOutput[0], scatOutput[1], scatOutput[2]) * 2, -1, 1);
	//W = normalize(clamp(float3(scatOutput[3], scatOutput[4], scatOutput[5]) * 2, -1, 1));

	//if (random() < Phi / accum) // selected center as scattered point
	{
		X = float3(0, 0, 0);
		W = float3(0, 0, 1);
	}

	x = mul(x, R);
	w = mul(w, R);
	X = mul(X, R);
	W = mul(W, R);
}



// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void VolumeScattering(float Extinction, float G, float Phi, float3 V, float3 P, inout RayPayload payload)
{
	payload.volBounces++;
	bool pathTrace = DispatchRaysIndex().x / (float)DispatchRaysDimensions().x < PathtracingPercentage;

	if (pathTrace)
	{
		if (random() < 1 - Phi)
		{
			payload.Importance = 0; // absorption
		}

		float3 newDir = GeneratePhase(G, -V);
		// Update scattered ray
		payload.ScatteredRay.Direction = newDir;
		payload.ScatteredRay.Position = P;
	}
	else {
		float r = MaximalRadius(P);

		float3 x, w, X, W;
		float Absorption, Accum;
		GenerateVariablesWithModel(G, Phi, -V, Extinction * r, x, w, X, W, Accum, Absorption);
		//GenerateVariables(G, Phi, -V, L, Extinction * r, x, w, X, Importance, Accum);

		if (random() < Absorption) // Absorption
		{
			payload.Importance = 0;
			//return;
		}

		payload.ScatteredRay.Direction = w;
		payload.ScatteredRay.Position = P + x * r;
	}
}


// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void SurfelScatteringWithoutAccumulation(float3 V, Vertex surfel, Material material, inout RayPayload payload)
{
	TransformWavelengthMaterial(material, payload.Importance);

	// Adding emissive and direct lighting
	float NdotV;
	bool invertNormal;
	float3 fN;
	float4 R, T;
	ComputeImpulses(V, surfel, material,
		NdotV,
		invertNormal,
		fN,
		R,
		T);

	float3 ratio;
	float3 direction;
	float pdf;
	RandomScatterRay(V, fN, R, T, material, ratio, direction, pdf);

	// Update gathered Importance to the viewer
	payload.Importance *= max(0, ratio);// / (1 - russianRoulette);
	// Update scattered ray
	payload.ScatteredRay.Direction = direction;
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN)) * 0.000001 * fN;
}



float3 DrawDistanceField(bool hit, float3 P, float3 V, Vertex surfel, Material material, int bounces)
{
	float d = length(surfel.P - P);

	float t = -log(max(0.0000001, 1 - random())) / Extinction[0];

	// initial scatter (primary rays)
	if (hit && t > d)
	{
		return 0;// FreeCell(P - V * d) ? float3(0, 1, 0) : float3(0, 0, 1);
	}
	else
	{
		P = P - V * t;

		//int3 cell = (P - MinimumGrid) * Size / (MaximumGrid - MinimumGrid);
		//if (any(cell < 0) || any(cell >= Size))
		//	return 0;
		//uint level = GetValueAt(cell);
		//return level <= 10 * (1 - Phi.x) ? 1000 : 0;

		float freeSize = MaximalRadius(P);
		if (freeSize < (1 << (int)(10 * (1 - Phi.x))) / 1024.0)
			return float3(1, freeSize, freeSize) * LightIntensity / 100;
		return 0;
	}
}

float3 SampleSkybox(float3 L) {

	//return float3(0, 0, 1);
	float3 BG_COLORS[5] =
	{
		float3(0.00f, 0.0f, 0.02f), // GROUND DARKER BLUE
		float3(0.01f, 0.05f, 0.2f), // HORIZON GROUND DARK BLUE
		float3(0.7f, 0.9f, 1.0f), // HORIZON SKY WHITE
		float3(0.1f, 0.3f, 1.0f),  // SKY LIGHT BLUE
		float3(0.01f, 0.1f, 0.7f)  // SKY BLUE
	};

	float BG_DISTS[5] =
	{
		-1.0f,
		-0.04f,
		0.0f,
		0.5f,
		1.0f
	};

	int N = 1000;
	float phongNorm = (N + 2) / (4 * pi);

	float3 col = BG_COLORS[0];
	//for (int i = 1; i < 5; i++)
	col = lerp(col, BG_COLORS[1], smoothstep(BG_DISTS[0], BG_DISTS[1], L.y));
	col = lerp(col, BG_COLORS[2], smoothstep(BG_DISTS[1], BG_DISTS[2], L.y));
	col = lerp(col, BG_COLORS[3], smoothstep(BG_DISTS[2], BG_DISTS[3], L.y));
	col = lerp(col, BG_COLORS[4], smoothstep(BG_DISTS[3], BG_DISTS[4], L.y));
	return col +
		pow(max(0, dot(L, LightDirection)), N) * phongNorm * LightIntensity;
}

float3 ComputePath(bool hit, float3 P, float3 V, Vertex surfel, Material material, int bounces, out int volBounces)
{
	RayPayload payload = (RayPayload)0;
	int cmp = PassCount % 3;// random() * 3;
	payload.Importance[cmp] = 3; // dividing 1 by 1/3.

	float d = length(surfel.P - P);

	float t = -log(max(0.0000001, 1 - random())) / Extinction[cmp];

	// initial scatter (primary rays)
	if (hit)
	{
		payload.bounces = 1;
		SurfelScatteringWithoutAccumulation(V, surfel, material, payload);
		if (material.Roulette.w > 0)
			payload.bounces = -1;
	}
	else {
		payload.AccRadiance = SampleSkybox(-V);
		payload.Importance = 0;
	}
	//else
	//	VolumeScattering(Extinction[cmp], G_Value[cmp], Phi[cmp], V, P - t * V, payload);

	payload.seed = getCurrentSeed();

	while (any(payload.Importance > 0.0000001))
	{
		RayDesc newRay;
		newRay.Origin = payload.ScatteredRay.Position;
		newRay.Direction = payload.ScatteredRay.Direction;
		newRay.TMin = 0.000001;
		newRay.TMax = 100.0;

		TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
	}
	volBounces = payload.volBounces;
	return payload.AccRadiance;
}
//float3 ComputePath(bool hit, float3 P, float3 V, Vertex surfel, Material material, int bounces)
//{
//	RayPayload payload = (RayPayload)0;
//	payload.Importance = 1;
//
//	float d = length(surfel.P - P);
//
//	float t = -log(max(0.0000001, 1 - random())) / Extinction;
//
//	// initial scatter (primary rays)
//	if (hit && t > d)
//		SurfelScatteringWithoutAccumulation(V, surfel, material, payload);
//	else
//		VolumeScattering(V, P - t * V, payload);
//
//	payload.seed = getCurrentSeed();
//
//	while (any(payload.Importance > 0.001))
//	{
//		RayDesc newRay;
//		newRay.Origin = payload.ScatteredRay.Position;
//		newRay.Direction = payload.ScatteredRay.Direction;
//		newRay.TMin = 0.001;
//		newRay.TMax = 10.0;
//
//		TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
//	}
//	return payload.AccRadiance;
//}


[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	// Start seed here...
	//setCurrentSeed(payload.seed);

	//if (payload.bounces < 0) // miss geometry
	//	payload.AccRadiance = 1 / 0;
	//else {
		payload.AccRadiance += payload.Importance * SampleSkybox(payload.ScatteredRay.Direction);
	//}
	payload.Importance = 0;

	//int cmp = (int)dot(payload.Importance > 0, float3(0, 1, 2));

	//float t = -log(max(0.0000001, 1 - random())) / Extinction[cmp];

	//float3 P = payload.ScatteredRay.Position + t * payload.ScatteredRay.Direction;

	//if (length(P) > VOL_DIST) // go outside envolving volume
	//	payload.Importance = 0;
	//else // hit media particle
	//	VolumeScattering(Extinction[cmp], G_Value[cmp], Phi[cmp], -payload.ScatteredRay.Direction, P, payload);

	//payload.seed = getCurrentSeed();
}


[shader("raygeneration")]
void PTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, 1, raysIndex, 0, PassCount);

	Vertex surfel;
	Material material;
	float3 V;
	float3 P;
	float2 coord = (raysIndex + 0.5) / raysDimensions;

	bool hit = GetPrimaryIntersection(raysIndex, coord, P, V, surfel, material);

	float4 ndcP = float4(2 * (raysIndex.xy + float2(random(), random())) / raysDimensions - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	V = -D;

	int volBounces = 0;
	float3 color;

	//color = DrawDistanceField(hit, P, V, surfel, material, PATH_TRACING_MAX_BOUNCES);

	color = ComputePath(hit, P, V, surfel, material, PATH_TRACING_MAX_BOUNCES, volBounces);

	//color = material.Roulette.w;

	if (any(isnan(color)))
		color = float3(0, 0, 0);

	if (CountSteps)
		color = float3 (volBounces / 256.0, (volBounces % 256) / 256.0, 0);// // GetColor(volBounces);

	AccumulateOutput(raysIndex, color);
}

[shader("closesthit")]
void PTScattering(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material, 0, 0);

	// Start seed here...
	setCurrentSeed(payload.seed);

	int cmp = (int)dot(payload.Importance > 0, float3(0, 1, 2));

	float d = length(surfel.P - payload.ScatteredRay.Position);

	float t = -log(max(0.000000000001, 1 - random())) / Extinction[cmp];

	if (t > d || payload.bounces >= 0) // hit opaque surface or travel outside volume
	{
		if (abs(payload.bounces) > PATH_TRACING_MAX_BOUNCES)
			//if (random() < 0.5)
				payload.Importance = 0;
			//else
			//	payload.Importance *= 2;

		if (any(payload.Importance)) {
			// use bounces sign to know if it is in a medium
			payload.bounces = sign(payload.bounces) * (abs(payload.bounces) + 1);
				// This is not a recursive closest hit but it will accumulate in payload
				// all the result of the scattering to this surface
			SurfelScattering(-WorldRayDirection(), surfel, material, payload);

			if (any(material.Specular) && material.Roulette.w > 0) // some fresnel
			{
				payload.bounces = sign(dot(surfel.N, payload.ScatteredRay.Direction))*abs(payload.bounces);

				//if (dot(surfel.N, -payload.ScatteredRay.Direction) > 0) // entering to the material
				//	payload.bounces = -abs(payload.bounces); // switch from medium
				//else
				//	payload.bounces = abs(payload.bounces);
			}
		}
	}
	else // hit media particle
		VolumeScattering(Extinction[cmp], G_Value[cmp], Phi[cmp], -WorldRayDirection(), payload.ScatteredRay.Position + t * payload.ScatteredRay.Direction, payload);

	payload.seed = getCurrentSeed();
}