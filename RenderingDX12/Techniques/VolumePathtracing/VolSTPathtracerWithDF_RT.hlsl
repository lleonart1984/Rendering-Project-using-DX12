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
	uint4 seed;
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

int split3(int value) {

	int ans = value & 0x3FF; // allow 10 bits only.

	ans = (ans | ans << 16) & 0xff0000ff;
	// poniendo 8 ceros entre cada grupo de 4 bits.
	// shift left 8 bits y despues hacer OR entre ans y 0001000000001111000000001111000000001111000000001111000000000000
	ans = (ans | ans << 8) & 0x0f00f00f;
	// poniendo 4 ceros entre cada grupo de 2 bits.
	// shift left 4 bits y despues hacer OR entre ans y 0001000011000011000011000011000011000011000011000011000011000011
	ans = (ans | ans << 4) & 0xc30c30c3;
	// poniendo 2 ceros entre cada bit.
	// shift left 2 bits y despues hacer OR entre ans y 0001001001001001001001001001001001001001001001001001001001001001
	ans = (ans | ans << 2) & 0x49249249;
	return ans;
}

int morton(int3 pos) {
	return split3(pos.x) | (split3(pos.y) << 1) | (split3(pos.z) << 2);
}

uint GetValueAt(int3 cell) {
	//int index = Size * (cell.y + cell.z * Size) + cell.x;
	int index = morton(cell);
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
		
		//float halfboxSize = pow(2, emptySizePower - 1) * 1.4142 - 0.5;// *0.5;
		float halfboxSize = pow(2, emptySizePower - 1) - 0.5;// *0.5;
		float3 minBox = (cell + 0.5 - halfboxSize) * cellSize + MinimumGrid;
		float3 maxBox = (cell + 0.5 + halfboxSize) * cellSize + MinimumGrid;

		float3 toMin = (P - minBox);
		float3 toMax = (maxBox - P);
		float3 m = min(toMin, toMax);
		return min(m.x, min(m.y, m.z));
	}
	return 1000;
	float3 proj = P <= MinimumGrid ? MinimumGrid : (P >= MaximumGrid ? MaximumGrid : P);
	return max(0, min(length(P - proj), VOL_DIST - length(P))); // distance to box and to the exterior of vol
}

void TransformWavelengthMaterial(inout Material material, float3 color) {
	//material.RefractionIndex = pow(material.RefractionIndex, color.x > 0 ? 1 : color.y > 0 ? 0.92 : 0.82);
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

//#include "../VolumeRendering/SphereScatteringWithDL.h"
#include "../VolumeRendering/IWAEScattering.h"

float sampleNormal(float mu, float logVar) {
	//return mu + gauss() * exp(logVar * 0.5);
	return mu + gauss() * exp(clamp(logVar, -20, 40) * 0.5);
}

bool GenerateVariablesWithNewModel(float G, float Phi, float3 win, float density, out float3 x, out float3 w)
{
	x = float3(0, 0, 0);
	w = win;

	float3 temp = abs(win.x) >= 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 winY = normalize(cross(temp, win));
	float3 winX = cross(win, winY);
	float rAlpha = random() * 2 * pi;
	float3x3 R = (mul(float3x3(
		cos(rAlpha), -sin(rAlpha), 0,
		sin(rAlpha), cos(rAlpha), 0,
		0, 0, 1), float3x3(winX, winY, win)));

	float codedDensity = density;// pow(density / 400.0, 0.125);

	float2 lenLatent = randomStdNormal2();
	// Generate length
	float lenInput[4];
	float lenOutput[2];
	lenInput[0] = codedDensity;
	lenInput[1] = G;
	lenInput[2] = lenLatent.x;
	lenInput[3] = lenLatent.y;
	lenModel(lenInput, lenOutput);

	float logN = max(log(1), sampleNormal(lenOutput[0], lenOutput[1]));
	float n = exp(logN);

	if (random() >= pow(Phi, n))
		return false;
	float4 pathLatent14 = randomStdNormal4();
	float pathLatent5 = randomStdNormal();
	// Generate path
	float pathInput[8];
	float pathOutput[6];
	pathInput[0] = codedDensity;
	pathInput[1] = G;
	pathInput[2] = logN;
	pathInput[3] = pathLatent14.x;
	pathInput[4] = pathLatent14.y;
	pathInput[5] = pathLatent14.z;
	pathInput[6] = pathLatent14.w;
	pathInput[7] = pathLatent5.x;
	pathModel(pathInput, pathOutput);
	float3 sampling = randomStdNormal3();
	float3 pathMu = float3(pathOutput[0], pathOutput[1], pathOutput[2]);
	float3 pathLogVar = float3(pathOutput[3], pathOutput[4], pathOutput[5]);
	float3 pathOut = clamp(pathMu + exp(clamp(pathLogVar, -20, 40) * 0.5) * sampling, -0.9999, 0.9999);
	float costheta = pathOut.x;
	float wt = pathOut.y;
	float wb = pathOut.z; 
	x = float3(0, sqrt(1 - costheta * costheta), costheta);
	float3 N = x;
	float3 B = float3(1, 0, 0);
	float3 T = cross(x, B);
	w = normalize(N * sqrt(max(0, 1 - wt * wt - wb * wb)) + T * wt + B * wb);
	x = mul(x, (R));
	w = mul(w, (R)); // move to radial space

	return true;// random() >= 1 - pow(Phi, n);
}

float randomKum(float alpha, float beta) {
	float u = random();
	return pow(1 - pow(1 - u, 1 / beta), 1 / alpha);
}

float AlphaFor(float sigma, float phi) {
	float a = 0.656585f;
	float b = -0.293104f;
	float c = 1.40309f;
	float d = 0.865207f;
	float e = 0.69545f;
	return (a * phi + b) * atan(log(sigma) - c) + (d * phi + e);
}

float BetaFor(float sigma, float phi) {
	return 2.94871f - 0.0783771f * atan(2.25587f - log(sigma)) + (0.11912f * phi - 0.210316f) * atan(0.392166f + log(sigma));
}

bool GenerateVariablesAnalytically(float Phi, float density, float3 win, out float3 x, out float3 w)
{
	x = randomDirection(-win);
	w = x;

	if (random() < 1 - Phi) // absorption at first scatteing
	{
		return false;
	}

	float prob = 1 - exp(-density);
	if (random() > prob) // single scattering
	{
		return true;
	}

	float mlogN = -0.590368 + 0.968372 * log(density * density + 5.02256);
	float vlogN = 0.34273;

	float n = max(2, (int)exp(round(gauss(mlogN, sqrt(vlogN)))));

	if (random() < (1 - pow(Phi, n - 1)))
	{
		return false;
	}

	float3 oB = abs(x.z) < 1 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 oT = normalize(cross(oB, x));
	oB = cross(oT, x);

	float gamma = pi * 0.5 * randomKum(AlphaFor(density, Phi), BetaFor(density, Phi));
	float randomRot = random() * 2 * pi;
	float cosgamma = cos(gamma);
	float singamma = sin(gamma);// sqrt(max(0, 1 - cosgamma * cosgamma));
	w = singamma * cos(randomRot) * oB + singamma * sin(randomRot) * oT + cosgamma * x;
	return true;
}


// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void VolumeScattering(float Extinction, float G, float Phi, float3 V, float3 P, inout RayPayload payload)
{
	bool pathTrace = DispatchRaysIndex().x / (float)DispatchRaysDimensions().x < PathtracingPercentage;

	if (pathTrace)
	{
		payload.volBounces++;

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
		payload.volBounces++;// += r == 0 ? 1 : 0;

		//if (Extinction * r < 2) {
		//	if (random() < 1 - Phi)
		//	{
		//		payload.Importance = 0; // absorption
		//	}

		//	float3 newDir = GeneratePhase(G, -V);
		//	// Update scattered ray
		//	payload.ScatteredRay.Direction = newDir;
		//	payload.ScatteredRay.Position = P;
		//}
		//else
		{

			float3 x, w, X, W;

			//if (abs(G) <= 0.001) // isotropic case
			//{
			//	/*float sigmaS = Phi * Extinction;
			//	float sigmaA = (1 - Phi) * Extinction;
			//	float correctedSigmaS = sigmaS * pow(1 - G, 6);
			//	float correctedExtinction = correctedSigmaS + sigmaA;
			//	float correctedPhi = r == 0 ? Phi : correctedSigmaS / correctedExtinction;*/
			//	
			//	if (!GenerateVariablesAnalytically(Phi, Extinction * r, -V, x, w))
			//	{
			//		payload.Importance = 0;
			//	}
			//	//float absorption = GenerateVariablesAnalytically2( Phi, Extinction * r, -V, x, w);
			//	//GenerateVariables(G, Phi, -V, L, Extinction * r, x, w, X, Importance, Accum);

			//	//if (random() < absorption) // Absorption
			//	//{
			//	//	payload.Importance = 0;
			//	//	//return;
			//	//}

			//}
			//else 
				//float Absorption, Accum;
				//GenerateVariablesWithModel(G, Phi, -V, Extinction * r, x, w, X, W, Accum, Absorption);
				////GenerateVariables(G, Phi, -V, L, Extinction * r, x, w, X, Importance, Accum);

				//if (random() < Absorption) // Absorption
				//{
				//	payload.Importance = 0;
				//	//return;
				//}

			if (!GenerateVariablesWithNewModel(G, Phi, -V, Extinction * r, x, w))
				payload.Importance = 0;

			payload.ScatteredRay.Direction = w;
			payload.ScatteredRay.Position = P + x * r;
		}
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
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN)) * 0.001 * fN;
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
		float3(0.00f, 0.0f, 0.1f), // GROUND DARKER BLUE
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

	int N = 30;
	float phongNorm = (N + 2) / (4 * pi);

	float3 col = BG_COLORS[0];
	//for (int i = 1; i < 5; i++)
	col = lerp(col, BG_COLORS[1], smoothstep(BG_DISTS[0], BG_DISTS[1], L.y));
	col = lerp(col, BG_COLORS[2], smoothstep(BG_DISTS[1], BG_DISTS[2], L.y));
	col = lerp(col, BG_COLORS[3], smoothstep(BG_DISTS[2], BG_DISTS[3], L.y));
	col = lerp(col, BG_COLORS[4], smoothstep(BG_DISTS[3], BG_DISTS[4], L.y));
	return //col + // *LightIntensity;// +
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
		//payload.AccRadiance = 0;// float3(1, 1, 1);// SampleSkybox(-V);
		payload.AccRadiance = SampleSkybox(-V);
		payload.Importance = 0;
	}
	//else
	//	VolumeScattering(Extinction[cmp], G_Value[cmp], Phi[cmp], V, P - t * V, payload);

	payload.seed = getRNG();

	while (any(payload.Importance > 0))
	{
		RayDesc newRay;
		newRay.Origin = payload.ScatteredRay.Position;
		newRay.Direction = payload.ScatteredRay.Direction;
		newRay.TMin = 0.00001;
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
	//if (payload.bounces != 0)
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

	//for (int i = 0; i < 10; i++)
	{
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
}

[shader("closesthit")]
void PTScattering(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material, 0, 0);

	// Start seed here...
	setRNG(payload.seed);

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
				payload.bounces = sign(dot(surfel.N, payload.ScatteredRay.Direction))*(abs(payload.bounces)-1);

				//if (dot(surfel.N, -payload.ScatteredRay.Direction) > 0) // entering to the material
				//	payload.bounces = -abs(payload.bounces); // switch from medium
				//else
				//	payload.bounces = abs(payload.bounces);
			}
		}
	}
	else // hit media particle
		VolumeScattering(Extinction[cmp], G_Value[cmp], Phi[cmp], -WorldRayDirection(), payload.ScatteredRay.Position + t * payload.ScatteredRay.Direction, payload);

	payload.seed = getRNG();
}