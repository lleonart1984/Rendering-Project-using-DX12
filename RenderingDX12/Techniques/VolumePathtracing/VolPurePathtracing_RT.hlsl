/// Implementation of an iterative and accumulative path-tracing.
/// Primary rays and shadow cast use GBuffer optimization
/// Path-tracing with next-event estimation.

#include "../CommonGI/Definitions.h"

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

#define VERTICES_REG				t1
#define MATERIALS_REG				t2
#define BACKGROUND_IMG_REG			t3
#define TEXTURES_REG				t5

#define TEXTURES_SAMPLER_REG		s0

#define OUTPUT_IMAGE_REG			u0
#define ACCUM_IMAGE_REG				u1

#define LIGHTING_CB_REG				b1
#define ACCUMULATIVE_CB_REG			b2
#define OBJECT_CB_REG				b3

//#include "../VolumeRendering/VAESphereScatteringBuffered.h"
#include "../VolumeRendering/VAESphereScattering.h"

#include "../CommonRT/CommonRTScenes.h"
#include "../CommonRT/CommonProgressive.h"
#include "../CommonRT/CommonOutput.h"
#include "../CommonGI/ScatteringTools.h"


StructuredBuffer<uint> Grid : register(t4);

cbuffer GridInfo : register(b5) {
	int Size;
	float3 MinimumGrid;
	float3 MaximumGrid;
}

cbuffer DebugInfo : register(b6) {
	int CountSteps;
}


struct Ray {
	float3 Position;
	float3 Direction;
};

struct RayPayload
{
	float Importance;
	float AccRadiance;
	int InMedium;
	Ray ScatteredRay;
	int bounces;
	int volBounces;
	uint4 seed;
};

cbuffer ParticipatingMedia : register(b4) {
	float3 Extinction;
	float3 G_Value;
	float3 Phi;
	float PathtracingPercentage;
}

cbuffer Transforming : register(b0) {
	row_major matrix FromProjectionToWorld;
}

#include "VolumeScattering.h"

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
	float3 cellSize = (MaximumGrid - MinimumGrid) / Size;
	int3 cell = (P - MinimumGrid) / cellSize;
	uint emptySizePower = GetValueAt(cell);

	//if (emptySizePower <= 0) // no empty cell
	//	return 0;

	float halfboxSize = (1 << (emptySizePower - 1));// *0.5;

	return emptySizePower == 0 ? 0 : (halfboxSize - 1)* cellSize;


	float3 minBox = (cell + 1 - halfboxSize) * cellSize + MinimumGrid;
	float3 maxBox = (cell + halfboxSize) * cellSize + MinimumGrid;

	float3 toMin = (P - minBox);
	float3 toMax = (maxBox - P);
	float3 m = min(toMin, toMax);
	return emptySizePower == 0 ? 0 : min(m.x, min(m.y, m.z));
}

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
//
//void GenerateVariablesWithModel(float G, float Phi, float3 win, float density, out float3 x, out float3 w, out float3 X, out float3 W, out float accum, out float absorption)
//{
//	x = float3(0, 0, 0);
//	w = win;
//
//	X = x;
//	W = w;
//
//	float prob = 1 - exp(-density);
//	if (random() >= prob) // single scattering
//	{
//		w = GeneratePhase(G, win);
//		x = w;
//		accum = Phi;
//		absorption = 1 - Phi;
//		return;
//	}
//
//	float3 temp = abs(win.x) >= 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
//	float3 winY = normalize(cross(temp, win));
//	float3 winX = cross(win, winY);
//	float rAlpha = random() * 2 * pi;
//	float3x3 R = (mul(float3x3(
//		cos(rAlpha), -sin(rAlpha), 0,
//		sin(rAlpha), cos(rAlpha), 0,
//		0, 0, 1), float3x3(winX, winY, win)));
//
//	// Generate path
//	float pathInput[12];
//	float pathOutput[4];
//	pathInput[0] = density;
//	pathInput[1] = Phi;
//	pathInput[2] = G;
//	pathInput[3] = gauss();
//	pathInput[4] = gauss();
//	pathInput[5] = gauss();
//	pathInput[6] = gauss();
//	pathInput[7] = gauss();
//	pathInput[8] = gauss();
//	pathInput[9] = gauss();
//	pathInput[10] = gauss();
//	pathInput[11] = gauss();
//	PathGen(pathInput, pathOutput);
//	float4 pO = float4(pathOutput[0], pathOutput[1], pathOutput[2], pathOutput[3]);
//	pO = clamp(pO * 2, -1, 1);
//	float costheta = pO.x;
//	float wt = pO.y;
//	float dwt = sqrt(1 - wt * wt);
//	float wb = clamp(pO.z * sign(random() - 0.5), -dwt, dwt);
//	x = float3(0, sqrt(max(0, 1 - costheta * costheta)), costheta);
//	float3 N = x;
//	float3 B = float3(1, 0, 0);
//	float3 T = cross(x, B);
//	w = (N * sqrt(max(0, 1 - wt * wt - wb * wb)) + T * wt + B * wb);
//	float n = 2.0 / max(0.000001, 1 - pO.w);
//	absorption = Phi == 1 ? 0 : 1 - pow(Phi, n);
//	accum = Phi == 1 ? n : Phi * absorption / (1 - Phi);
//
//	//float scatInput[16];
//	//float scatOutput[6];
//	//scatInput[0] = density;
//	//scatInput[1] = Phi;
//	//scatInput[2] = G;
//	//scatInput[3] = pathOutput[0];
//	//scatInput[4] = pathOutput[1];
//	//scatInput[5] = pathOutput[2];
//	//scatInput[6] = pathOutput[3];
//	//scatInput[7] = gauss();
//	//scatInput[8] = gauss();
//	//scatInput[9] = gauss();
//	//scatInput[10] = gauss();
//	//scatInput[11] = gauss();
//	//scatInput[12] = gauss();
//	//scatInput[13] = gauss();
//	//scatInput[14] = gauss();
//	//scatInput[15] = gauss();
//	////ScatGen(scatInput, scatOutput);
//	//X = clamp(float3(scatOutput[0], scatOutput[1], scatOutput[2]) * 2, -1, 1);
//	//W = normalize(clamp(float3(scatOutput[3], scatOutput[4], scatOutput[5]) * 2, -1, 1));
//
//	//if (random() < Phi / accum) // selected center as scattered point
//	{
//		X = float3(0, 0, 0);
//		W = float3(0, 0, 1);
//	}
//
//	x = mul(x, R);
//	w = mul(w, R);
//	X = mul(X, R);
//	W = mul(W, R);
//}


bool GenerateVariablesWithNewModel(float G, float Phi, float3 win, float density, out float3 x, out float3 w)
{
	x = float3(0, 0, 0);
	w = win;

	float prob = 1 - exp(-density);
	if (random() >= prob) // single scattering
	{
		w = GeneratePhase(G, win);
		x = w;
		return random() < Phi;
	}

	float3 temp = abs(win.x) >= 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 winY = normalize(cross(temp, win));
	float3 winX = cross(win, winY);
	float rAlpha = random() * 2 * pi;
	float3x3 R = (mul(float3x3(
		cos(rAlpha), -sin(rAlpha), 0,
		sin(rAlpha), cos(rAlpha), 0,
		0, 0, 1), float3x3(winX, winY, win)));

	// Generate length
	float lenInput[4];
	float lenOutput[1];
	lenInput[0] = exp(-density / 10);
	lenInput[1] = G;
	lenInput[2] = gauss();
	lenInput[3] = gauss();
	//lenInput[4] = gauss();
	//lenInput[5] = gauss();
	//lenInput[6] = gauss();
	//lenInput[7] = gauss();
	LengthGen(lenInput, lenOutput);

	int n = (int)max(1, round(exp(lenOutput[0])));

	if (random() >= pow(Phi, n))
		return false;

	// Generate path
	float pathInput[12];
	float pathOutput[3];
	pathInput[0] = exp(-density / 10);
	pathInput[1] = G;
	pathInput[2] = log(n);
	pathInput[3] = gauss();
	pathInput[4] = gauss();
	pathInput[5] = gauss();
	pathInput[6] = gauss();
	pathInput[7] = gauss();
	pathInput[8] = gauss();
	pathInput[9] = gauss();
	pathInput[10] = gauss();
	pathInput[11] = gauss();
	/*pathInput[12] = gauss();
	pathInput[13] = gauss();
	pathInput[14] = gauss();
	pathInput[15] = gauss();*/
	PathGen(pathInput, pathOutput);
	float3 pO = clamp(float3(pathOutput[0], pathOutput[1], pathOutput[2]), -0.9999999, 0.9999999);
	float costheta = pO.x;
	float wt = pO.y;
	float dwt = sqrt(1 - wt * wt);
	float wb = clamp(pO.z, -dwt, dwt);
	x = float3(0, sqrt(1 - costheta * costheta), costheta);
	float3 N = x;
	float3 B = float3(1, 0, 0);
	float3 T = cross(x, B);
	w = normalize(N * sqrt(max(0, 1 - wt * wt - wb * wb)) + T * wt + B * wb);

	x = mul(x, R);
	w = mul(w, R);

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

	float n = max(2, (int)exp(gauss(mlogN, sqrt(vlogN))));

	if (random() < (1 - pow(Phi, n)))
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

		payload.Importance *= random() >= 1 - Phi; // absorption

		float3 newDir = GeneratePhase(G, -V);
		// Update scattered ray
		payload.ScatteredRay.Direction = newDir;
		payload.ScatteredRay.Position = P;
	}
	else {

		float r = MaximalRadius(P);

		payload.volBounces++;// += r == 0 ? 1 : 0;

		float3 x, w, X, W;

		/*float a = 1 - Phi;
		float s = (1 - G) * Extinction * Phi;
		Extinction = a + s;
		Phi = s / Extinction;*/

		if (abs(G) <= 0.001) // isotropic case
		{
			/*float sigmaS = Phi * Extinction;
			float sigmaA = (1 - Phi) * Extinction;
			float correctedSigmaS = sigmaS * pow(1 - G, 6);
			float correctedExtinction = correctedSigmaS + sigmaA;
			float correctedPhi = r == 0 ? Phi : correctedSigmaS / correctedExtinction;*/
			
			payload.Importance *= GenerateVariablesAnalytically(Phi, Extinction * r, -V, x, w);
			
			//float absorption = GenerateVariablesAnalytically2( Phi, Extinction * r, -V, x, w);
			//GenerateVariables(G, Phi, -V, L, Extinction * r, x, w, X, Importance, Accum);

			//if (random() < absorption) // Absorption
			//{
			//	payload.Importance = 0;
			//	//return;
			//}

		}
		else
		{
			payload.Importance *= GenerateVariablesWithNewModel(G, Phi, -V, Extinction * r, x, w);

			//float Absorption, Accum;
			//GenerateVariablesWithModel(G, Phi, -V, Extinction * r, x, w, X, W, Accum, Absorption);
			//GenerateVariables(G, Phi, -V, L, Extinction * r, x, w, X, Importance, Accum);

			//payload.Importance *= random() >= Absorption;
		}

		payload.ScatteredRay.Direction = w;
		payload.ScatteredRay.Position = P + x * r;
	}
}

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void SurfelScattering(float3 V, Vertex surfel, Material material, inout RayPayload payload)
{
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
	payload.Importance *= max(0, ratio[PassCount % 3]);// / (1 - russianRoulette);
	// Update scattered ray
	payload.ScatteredRay.Direction = direction;
	payload.ScatteredRay.Position = surfel.P + sign(dot(direction, fN)) * 0.0000001 * fN;
}

float3 ComputePath(float3 P, float3 D, out int volCount)
{
	RayPayload payload = (RayPayload)0;
	int cmp = PassCount % 3;// random() * 3;
	payload.Importance = 3; // dividing 1 by 1/3.

	payload.ScatteredRay.Position = P;
	payload.ScatteredRay.Direction = D;
	payload.seed = getRNG();
	payload.bounces = 0;

	while (payload.Importance > 0)
	{
		RayDesc newRay;
		newRay.Origin = payload.ScatteredRay.Position;
		newRay.Direction = payload.ScatteredRay.Direction;
		newRay.TMin = 0.0000001;
		newRay.TMax = 100.0;

		TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newRay, payload);
	}
	float3 result = 0;
	result[cmp] = payload.AccRadiance;
	volCount = payload.volBounces;
	return result;
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

	
	float3 col = BG_COLORS[0];
	//for (int i = 1; i < 5; i++)
	col = lerp(col, BG_COLORS[1], smoothstep(BG_DISTS[0], BG_DISTS[1], L.y));
	col = lerp(col, BG_COLORS[2], smoothstep(BG_DISTS[1], BG_DISTS[2], L.y));
	col = lerp(col, BG_COLORS[3], smoothstep(BG_DISTS[2], BG_DISTS[3], L.y));
	col = lerp(col, BG_COLORS[4], smoothstep(BG_DISTS[3], BG_DISTS[4], L.y));
	return 0;// col;// + // *LightIntensity;// +
		
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	int N = 30;
	float phongNorm = (N + 2) / (4 * pi);
	float3 phongLight = pow(max(0, dot(payload.ScatteredRay.Direction, LightDirection)), N) * phongNorm * LightIntensity;

	int cmp = PassCount % 3;
	payload.AccRadiance += payload.Importance * SampleSkybox(payload.ScatteredRay.Direction)[cmp];
	
	if (payload.bounces >= 1)
		payload.AccRadiance += payload.Importance * phongLight[cmp];
	
	payload.Importance = 0;
}

[shader("raygeneration")]
void PTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, 1, raysIndex, 0, PassCount);

	float4 ndcP = float4(2 * (raysIndex.xy + float2(random(), random())) / raysDimensions - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);
	
	int volBounces;
	float3 color = ComputePath(O, D, volBounces);

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
	setRNG(payload.seed);

	int cmp = PassCount % 3;

	float d = length(surfel.P - payload.ScatteredRay.Position);

	float t = -log(max(0.000000000001, 1 - random())) / Extinction[cmp];

	if (t > d || payload.InMedium == 0) // hit opaque surface or travel outside volume
	{
		payload.Importance *= (payload.bounces <= PATH_TRACING_MAX_BOUNCES);

		//if (payload.Importance > 0) {
			// use bounces sign to know if it is in a medium
			// This is not a recursive closest hit but it will accumulate in payload
			// all the result of the scattering to this surface
			SurfelScattering(-WorldRayDirection(), surfel, material, payload);

			bool getIn = dot(surfel.N, payload.ScatteredRay.Direction) < 0;
			payload.InMedium = getIn;
			payload.bounces += (1 - getIn);
		//}
	}
	else // hit media particle
		VolumeScattering(Extinction[cmp], G_Value[cmp], Phi[cmp], -WorldRayDirection(), payload.ScatteredRay.Position + t * payload.ScatteredRay.Direction, payload);

	payload.seed = getRNG();
}