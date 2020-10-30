/// Implementation of an iterative and accumulative path-tracing.
/// Primary rays and shadow cast use GBuffer optimization
/// Path-tracing with next-event estimation.

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);

#define VERTICES_REG				t1
#define MATERIALS_REG				t2
#define BACKGROUND_IMG_REG			t3
StructuredBuffer<uint> Grid : register(t4);
#define TEXTURES_REG				t5

#define TEXTURES_SAMPLER_REG		s0
#define SHADOW_MAP_SAMPLER_REG		s1

#define OUTPUT_IMAGE_REG			u0
#define ACCUM_IMAGE_REG				u1

#define LIGHTING_CB_REG				b0
#define ACCUMULATIVE_CB_REG			b1
#define OBJECT_CB_REG				b6

#include "../CommonRT/CommonRTScenes.h"
#include "../CommonRT/CommonProgressive.h"
#include "../CommonRT/CommonOutput.h"
#include "../CommonGI/ScatteringTools.h"

struct Ray {
	float3 Position;
	float3 Direction;
};

struct RayPayload // Only used for raycasting
{
	int TriangleIndex;
	int MaterialIndex;
	float3 Barycentric;
};


cbuffer ParticipatingMedia : register(b2) {
	float3 Extinction;
	float3 G_Value;
	float3 Phi;
	float PathtracingPercentage;
}

cbuffer Transforming : register(b3) {
	row_major matrix FromProjectionToWorld;
}

cbuffer GridInfo : register(b4) {
	int Size;
	float3 MinimumGrid;
	float3 MaximumGrid;
}

cbuffer DebugInfo : register(b5) {
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
void SurfelScattering(inout float3 x, inout float3 w, inout float3 importance, Vertex surfel, Material material)
{
	float3 V = -w;

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
	importance *= max(0, ratio);// / (1 - russianRoulette);
	// Update scattered ray
	w = direction;
	x = surfel.P + sign(dot(direction, fN)) * 0.0001 * fN;
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

//#include "../VolumeRendering/VAESphereScattering.h"
//#include "../VolumeRendering/IWAEScattering.h"
//#include "../VolumeRendering/CVAEScattering.h"
#include "../VolumeRendering/NewCVAEScattering.h"

float sampleNormal(float mu, float logVar) {
	//return mu + gauss() * exp(logVar * 0.5);
	return mu + gauss() * exp(clamp(logVar, -40, 76) * 0.5);
}

bool GenerateVariablesWithNewModel(float G, float Phi, float3 win, float density, out float3 x, out float3 w)
{
	x = float3(0, 0, 0);
	w = win;

	float3 temp = abs(win.x) >= 0.9999 ? float3(0, 0, 1) : float3(1, 0, 0);
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

	float logN = max(0, sampleNormal(lenOutput[0], lenOutput[1]));
	float n = floor(exp(logN));
	logN = log(n);

	if (random() >= pow(Phi, n))
		return false;

	float3 pathLatent13 = randomStdNormal3();
	//float pathLatent5 = randomStdNormal();
	// Generate path
	float pathInput[6];
	float pathOutput[6];
	pathInput[0] = codedDensity;
	pathInput[1] = G;
	pathInput[2] = logN;
	pathInput[3] = pathLatent13.x;
	pathInput[4] = pathLatent13.y;
	pathInput[5] = pathLatent13.z;
	//pathInput[6] = pathLatent14.w;
	//pathInput[7] = pathLatent5.x;
	pathModel(pathInput, pathOutput);
	float3 sampling = randomStdNormal3();
	float3 pathMu = float3(pathOutput[0], pathOutput[1], pathOutput[2]);
	float3 pathLogVar = float3(pathOutput[3], pathOutput[4], pathOutput[5]);
	float3 pathOut = clamp(pathMu + exp(clamp(pathLogVar, -40, 76) * 0.5) * sampling, -0.9999, 0.9999);
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

// Represents a single bounce of path tracing
// Will accumulate emissive and direct lighting modulated by the carrying importance
// Will update importance with scattered ratio divided by pdf
// Will output scattered ray to continue with
void VolumeScattering(inout float3 x, inout float3 w, inout float3 importance, float Extinction, float G, float Phi)
{
	bool pathTrace = DispatchRaysIndex().x / (float)DispatchRaysDimensions().x < PathtracingPercentage;

	importance *= (random() >= 1 - Phi);

	// Update scattered ray
	w = GeneratePhase(G, w);

	[branch]
	if (!pathTrace) // Try to skip multiple scattering
	{
		float r = MaximalRadius(x);

		float density = Extinction * r;

		//r = density < 5 ? 0 : r;
		//density = density < 5 ? 0 : density;

		float3 _x, _w;

		bool passMulti = GenerateVariablesWithNewModel(G, Phi, w, Extinction * r, _x, _w);

		bool multiScattering = random() > exp(-density);
		x += (multiScattering ? _x : w) * r;
		w = multiScattering ? _w : w;
		importance *= multiScattering ? passMulti : 1;
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

	int N = 40;
	float phongNorm = (N + 2) / (4 * pi);

	float3 col = BG_COLORS[0];
	//for (int i = 1; i < 5; i++)
	col = lerp(col, BG_COLORS[1], smoothstep(BG_DISTS[0], BG_DISTS[1], L.y));
	col = lerp(col, BG_COLORS[2], smoothstep(BG_DISTS[1], BG_DISTS[2], L.y));
	col = lerp(col, BG_COLORS[3], smoothstep(BG_DISTS[2], BG_DISTS[3], L.y));
	col = lerp(col, BG_COLORS[4], smoothstep(BG_DISTS[3], BG_DISTS[4], L.y));
	return 0;// col;// + // *LightIntensity;// +
		//pow(max(0, dot(L, LightDirection)), N) * phongNorm * LightIntensity;
}

float3 SampleLight(float3 L)
{
	int N = 10;
	float phongNorm = (N + 2) / (4 * pi);
	return pow(max(0, dot(L, LightDirection)), N) * phongNorm * LightIntensity;
}

bool Intersect(float3 P, float3 D, out int tIndex, out int mIndex, out float3 barycenter) {
	RayPayload payload = (RayPayload)0;
	RayDesc ray;
	ray.Origin = P;
	ray.Direction = D;
	ray.TMin = 0;
	ray.TMax = 100.0;
	TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, ray, payload);
	tIndex = payload.TriangleIndex;
	mIndex = payload.MaterialIndex;
	barycenter = payload.Barycentric;
	return tIndex >= 0;
}

float3 ComputePath(float3 O, float3 D, out int volBounces)
{
	int cmp = PassCount % 3;// random() * 3;
	float3 importance = 0;
	importance[cmp] = 3;
	float3 x = O;
	float3 w = D;

	bool inMedium = false;

	volBounces = 0;
	int bounces = 0;

	while (importance[cmp] > 0)
	{
		volBounces++;

		int tIndex;
		int mIndex;
		float3 coords;
		[branch]
		if (!Intersect(x, w, tIndex, mIndex, coords)) // 
			return importance * (SampleSkybox(w) + SampleLight(w) * (bounces > 0));

		Vertex surfel;
		Material material;
		GetHitInfo(coords, tIndex, mIndex, surfel, material, 0, 0);
		float d = length(surfel.P - x);
		float t = -log(max(0.000000000001, 1 - random())) / Extinction[cmp];

		[branch]
		if (t > d || !inMedium) // surface scattering
		{
			bounces++;

			importance *= (bounces <= PATH_TRACING_MAX_BOUNCES);

			SurfelScattering(x, w, importance, surfel, material);
			if (any(material.Specular) && material.Roulette.w > 0) // some fresnel
				inMedium = dot(surfel.N, w) < 0;
		}
		else // volume scattering
		{
			x += t * w;
			VolumeScattering(x, w, importance, Extinction[cmp], G_Value[cmp], Phi[cmp]);
		}
	}

	return 0;
}

[shader("miss")]
void EnvironmentMap(inout RayPayload payload)
{
	payload.TriangleIndex = -1;
}

[shader("raygeneration")]
void PTMainRays()
{
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();
	StartRandomSeedForRay(raysDimensions, 1, raysIndex, 0, PassCount);

	float2 coord = (raysIndex.xy + float2(random(), random())) / raysDimensions;

	float4 ndcP = float4(2 * coord - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	//for (int i = 0; i < 10; i++)
	{
		int volBounces = 0;
		float3 color;

		//color = DrawDistanceField(hit, P, V, surfel, material, PATH_TRACING_MAX_BOUNCES);

		color = ComputePath(O, D, volBounces);

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
	payload.Barycentric = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	payload.TriangleIndex = objectInfo.TriangleOffset + PrimitiveIndex();
	payload.MaterialIndex = objectInfo.MaterialIndex;
}