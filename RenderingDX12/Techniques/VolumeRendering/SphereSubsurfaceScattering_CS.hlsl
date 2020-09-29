#include "../CommonGI/Parameters.h"
#include "../Randoms/RandomUsed.h"

cbuffer Camera : register(b0) {
	row_major matrix FromProjectionToWorld;
};

cbuffer ScatteringInfo : register(b1) {
	float3 Sigma;
	float3 G;
	float3 Phi;
	float PathtracingPercentage;
}

cbuffer DebugInfo : register(b2) {
	int CountSteps;
}

#define one_minus_g2 (1.0 - G * G)
#define one_plus_g2 (1.0 + G * G)
#define one_over_2g (0.5 / G)

#define USE_DIRECT_LIGHT

//#ifndef pi
//#define pi 3.1415926535897932384626433832795
//#endif

cbuffer LightingInfo : register(b3) {
	float3 LightPosition;
	float3 LightIntensity;
	float3 LightDirection;
}

cbuffer PathtracingInfo : register(b4) {
	int PassCount;
};

float EvalPhase(float G, float3 D, float3 L) {
	if (abs(G) <= 0.001)
		return 0.25 / pi;

	float cosTheta = dot(D, L);
	return 0.25 / pi * (one_minus_g2) / pow(one_plus_g2 - 2 * G * cosTheta, 1.5);
}

float invertcdf(float G, float xi) {
	float t = (one_minus_g2) / (1.0f - G + 2.0f * G * xi);
	return one_over_2g * (one_plus_g2 - t * t);
}
float calcpdf(float G, float costheta) {
	return 0.25 * one_minus_g2 / (pi * pow(one_plus_g2 - 2.0f * G * costheta,
		1.5f));
}

void CreateOrthonormalBasis(float3 D, out float3 B, out float3 T) {
	float3 other = abs(D.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	B = normalize(cross(other, D));
	T = normalize(cross(D, B));
}

void CreateOrthonormalBasis(float3 D, float3 other, out float3 B, out float3 T) {
	if (abs(dot(D, other)) == 1)
	{
		float3 other1 = float3(D.y, -D.x, 0);
		float3 other2 = float3(0, -D.z, D.y);
		other = any(other1) ? other1 : other2;
	}

	B = normalize(cross(D, other));
	T = normalize(cross(B, D));
}

//float random();

float3 GeneratePhase(float G, float3 D) {
	if (abs(G) <= 0.001) // isotropic
		return randomDirection(-D);

	float phi = random() * 2 * pi;
	float cosTheta = invertcdf(G, random());
	float sinTheta = sqrt(max(0, 1.0f - cosTheta * cosTheta));

	float3 t0, t1;
	CreateOrthonormalBasis(D, t0, t1);

	return sinTheta * sin(phi) * t0 + sinTheta * cos(phi) * t1 +
		cosTheta * D;
}

RWTexture2D<float3> Accumulation	: register(u0);
RWTexture2D<float3> Output			: register(u1);

//float3 SampleSkybox(float3 L) {
//
//	int N = 1000;
//	float phongNorm = (N + 2) / (4 * pi);
//
//	float azim = L.y.x;
//	azim = pow(max(0, azim), 0.01) * sign(azim);
//	return lerp(float3(L.x * L.x, 1, 1), float3(0, L.z * L.z, 0), 0.5 - azim * 0.5)
//		 +pow(max(0, dot(L, LightDirection)), N) * phongNorm * 25;
//}

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

	int N = 300;
	float phongNorm = (N + 2) / (4 * pi);

	float3 col = BG_COLORS[0];
	//for (int i = 1; i < 5; i++)
	col = lerp(col, BG_COLORS[1], smoothstep(BG_DISTS[0], BG_DISTS[1], L.y));
	col = lerp(col, BG_COLORS[2], smoothstep(BG_DISTS[1], BG_DISTS[2], L.y));
	col = lerp(col, BG_COLORS[3], smoothstep(BG_DISTS[2], BG_DISTS[3], L.y));
	col = lerp(col, BG_COLORS[4], smoothstep(BG_DISTS[3], BG_DISTS[4], L.y));

	//#ifdef USE_DIRECT_LIGHT
	//	return 0;
	//#else
	return pow(max(0, dot(L, LightDirection)), N) * phongNorm * LightIntensity;
	//#endif

	return 0;//col + 
		//pow(max(0, dot(L, LightDirection)), N) * phongNorm * LightIntensity;
}

bool unitSphereIntersections(float3 p, float3 d, out float inT, out float outT) {
	float3 D = d;
	float3 F = p;
	float a = dot(D, D);
	float b = 2 * dot(F, D);
	float c = dot(F, F) - 1;

	float Disc = b * b - 4 * a * c;

	inT = 0;
	outT = 0;

	if (Disc < 0)
		return false;

	inT = (-b - sqrt(Disc)) / (2 * a);
	outT = (-b + sqrt(Disc)) / (2 * a);
	inT = max(0, inT);
	outT = max(0, outT);
	return true;
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

float ComputeFresnel(float NdotL, float ratio)
{
	float oneMinusRatio = 1 - ratio;
	float onePlusRatio = 1 + ratio;
	float divOneMinusByOnePlus = oneMinusRatio / onePlusRatio;
	float f = divOneMinusByOnePlus * divOneMinusByOnePlus;
	return (f + (1.0 - f) * pow((1.0 - NdotL), 5));
}

float3 PathtraceNoDirectLight(float G, float Phi, float Sigma, float3 P, float3 D, out int bounces) {
	bounces = 0;
	float3 A = 0;
	[loop]
	while (true)
	{
		bounces++;

		float t = abs(Sigma) <= 0.00001 ? 10000 : -log(max(0.0000001, 1 - random())) / Sigma;

		float d = exitSphere(P, D);

		P += min(t, d) * D;
		bool scatter = true;
		//[branch]
		if (t >= d)
		{
			if (random() < 0.5) {
				float3 N = normalize(P);
				float F = ComputeFresnel(dot(-N, -D), 1.76);
				float3 T = refract(D, -N, 1.76);
				F = any(T) ? F : 1;
				if (random() < 1 - F) {
					//D = T;
					return 0;
				}
				else
				{
					D = reflect(D, -N);
					P *= 0.9999;
					scatter = false;
				}
			}
			else {
				return SampleSkybox(D);
			}
		}

		if (scatter) {
			//[branch]
			if (random() < 1 - Phi) // absortion
				return A;

			D = GeneratePhase(G, D);
		}
	}
}


float3 Pathtrace(float G, float Phi, float Sigma, float3 P, float3 D, out int bounces) {
	bounces = 0;
	float3 A = 0;
	[loop]
	while (true)
	{
		bounces++;

		float t = abs(Sigma) <= 0.00001 ? 10000 : -log(max(0.0000001, 1 - random())) / Sigma;

		float d = exitSphere(P, D);

		P += min(t, d) * D;
		bool scatter = true;
		//[branch]
		if (t >= d)
		{
			float3 N = normalize(P);
			float F = ComputeFresnel(dot(-N, -D), 1.76);
			float3 T = refract(D, -N, 1.76);
			F = any(T) ? F : 1;
			if (random() < 1 - F) {
				D = T;
#ifdef USE_DIRECT_LIGHT
				return A;// +SampleSkybox(D);
#else
				return A + SampleSkybox(D);
#endif
			}
			else
			{
				D = reflect(D, -N);
				P *= 0.9999;
				scatter = false;
			}
		}

		if (scatter) {
			//[branch]
			if (random() < 1 - Phi) // absortion
				return A;


#ifdef USE_DIRECT_LIGHT
			float3 Ld = LightDirection;
			d = exitSphere(P, LightDirection);
			A += exp(-d * Sigma) * EvalPhase(G, D, LightDirection) * LightIntensity / (2 * pi);
#endif

			D = GeneratePhase(G, D);
		}
	}
}

void GenerateVariables(float G, float Phi, float3 win, float density, out float3 x, out float3 w, out float3 X, out float3 W, out float accum, out float importance) {
	//float prob = 1 - exp(-density);

	x = 0;
	w = win;

	X = x;
	W = w;
	importance = Phi;

	w = GeneratePhase(G, w);
	accum = Phi;

	float xi = 1 - random();
	float t = abs(density) <= 0.001 ? 10000 : -log(max(0.00000001, xi)) / density;

	if (t > 1) // single scattering
	{
		x = w;
		return;
	}

	x = x + w * t;
	[loop]
	while (true)
	{
		importance *= Phi;
		float currentPdf = importance;
		accum += currentPdf;
		if (random() < currentPdf / accum)
		{
			X = x;
			W = w;
		}

		w = GeneratePhase(G, w);

		float d = exitSphere(x, w);

		xi = 1 - random();
		t = abs(density) <= 0.001 ? 10000 : -log(max(0.0000000001, xi)) / density;

		[branch]
		if (t > d)
		{
			x += d * w;
			return;
		}

		x += t * w;
	}
}

int GenerateN(float G, float Phi, float density) {
	//float prob = 1 - exp(-density);

	float3 x = 0;
	float3 w = float3(0,0,1);

	w = GeneratePhase(G, w);

	float xi = 1 - random();
	float t = abs(density) <= 0.001 ? 10000 : -log(max(0.00000001, xi)) / density;

	if (t > 1) // single scattering
	{
		return 1;
	}

	int n = 1;

	x = x + w * t;
	[loop]
	while (true)
	{
		w = GeneratePhase(G, w);
		n++;

		float d = exitSphere(x, w);

		xi = 1 - random();
		t = abs(density) <= 0.001 ? 10000 : -log(max(0.0000000001, xi)) / density;

		[branch]
		if (t > d)
		{
			x += d * w;
			return n;
		}

		x += t * w;
	}
}


#include "../VolumeRendering/IWAEScattering.h"

float sampleNormal(float mu, float logVar) {
	//return mu + gauss() * exp(logVar * 0.5);
	return mu + gauss()*exp(clamp(logVar, -7, 70) * 0.5);
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

	float codedDensity = pow(density/400.0, 0.125);

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
	float n = exp(logN);

	//float logN = log(GenerateN(G, Phi, density));
	//float n = exp(logN);

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
	float3 pathOut = clamp(pathMu + exp(pathLogVar * 0.5) * sampling, -0.9999, 0.9999);
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

bool GenerateFullVariablesWithNewModel(float G, float Phi, float3 win, float density, out float3 x, out float3 w, out float factor, out float3 X, out float3 W)
{
	x = float3(0, 0, 0);
	w = win;
	X = x;
	W = win;
	factor = 1;

	float3 temp = abs(win.x) >= 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 winY = normalize(cross(temp, win));
	float3 winX = cross(win, winY);
	float rAlpha = random() * 2 * pi;
	float3x3 R = (mul(float3x3(
		cos(rAlpha), -sin(rAlpha), 0,
		sin(rAlpha), cos(rAlpha), 0,
		0, 0, 1), float3x3(winX, winY, win)));

	float codedDensity = pow(density / 400.0, 0.125);

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
	float3 pathOut = clamp(pathMu + exp(pathLogVar * 0.5) * sampling, -0.9999, 0.9999);
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

	// Generate Scattering

	float scatInput[16];
	float scatOutput[12];
	scatInput[0] = codedDensity;
	scatInput[1] = G;
	scatInput[2] = pow(1.00000 - Phi, 0.1);
	scatInput[3] = logN;
	scatInput[4] = costheta;
	scatInput[5] = wt;
	scatInput[6] = wb;
	scatInput[7] = gauss();
	scatInput[8] = gauss();
	scatInput[9] = gauss();
	scatInput[10] = gauss();
	scatInput[11] = gauss();
	scatInput[12] = gauss();
	scatInput[13] = gauss();
	scatInput[14] = gauss();
	scatInput[15] = gauss();
	/*scatInput[16] = gauss();
	scatInput[17] = gauss();
	scatInput[18] = gauss();
	scatInput[19] = gauss();*/
	scatModel(scatInput, scatOutput);

	/*float accum = 0;
	for (int i = 1; i <= n; i++)
		accum += pow(Phi, i);*/

	float accum = Phi >= 0.9999 ? n : Phi * (1 - pow(Phi, n)) / (1 - Phi);

	float probFirstScattering = Phi / accum;

	//if (false)
	//if (random() < 1 - probFirstScattering)
	{
		X = clamp(float3(
			sampleNormal(scatOutput[0], scatOutput[6]), 
			sampleNormal(scatOutput[1], scatOutput[7]),
			sampleNormal(scatOutput[2], scatOutput[8])), -1, 1);
		X /= max(1, length(X));
		W = normalize(clamp(float3(
			sampleNormal(scatOutput[3], scatOutput[9]),
			sampleNormal(scatOutput[4], scatOutput[10]),
			sampleNormal(scatOutput[5], scatOutput[11])), -1, 1));
		//W /= max(0.000001, length(W));
		X = mul(X, (R));
		W = mul(W, (R)); // move to radial space
	} // second scatter comes from nn

	factor = accum / pow(Phi, n);

	return true;// random() >= 1 - pow(Phi, n);
}

float3 SpherePathtracing(float G, float Phi, float Sigma, float3 x, float3 w, out int bounces)
{
	bounces = 0;
	float3 A = 0;
	[loop]
	while (true) {
		bounces++;
		float t = abs(Sigma) <= 0.001 ? 10000 : -log(max(0.0000001, 1 - random())) / Sigma;
		float d = exitSphere(x, w);

		x += min(t, d) * w;
		bool scatter = true;

		if (t >= d)
		{
			float3 N = normalize(x);
			float F = ComputeFresnel(dot(-N, -w), 1.76);
			float3 T = refract(w, -N, 1.76);
			F = any(T) ? F : 1;
			if (random() < 1 - F)
			{
				w = T;
				return A + SampleSkybox(w);
			}
			else
			{
				w = reflect(w, -N);
				x *= 0.9999;
				scatter = false;
			}
		}

		if (scatter) {
			float r = max(0, 1 - length(x));
			//float r = min(1 - length(x), 50 / Sigma);
			float density = Sigma * r;

			float3 X, W;
			float3 ox, ow;
			float accum;
			float importance;

			[branch]
			if (!GenerateVariablesWithNewModel(G, Phi, w, density, ox, ow))
				return A;

			//GenerateVariables(G, Phi, w, density, ox, ow, X, W, accum, importance);

			//float n = s * 2;
			//float absorption = Phi == 1 ? 0 : 1 - 1.0 / (n * log(Phi));

			//float absorption = 1 - pow(Phi, int(xd * density / 0.7764803));

			//if (random() < 1 - importance) // Absortion
			//	return A;

			//A += LightIntensity * exp(-(exitSphere(x + X * r, LightDirection)) * Sigma) * EvalPhase(G, W, LightDirection) * accum * I;
			//float accum = Phi == 1 ? n : Phi * pow(1 - Phi, n) / (1 - Phi);
			//I *= importance;

			x += ox * r;
			w = ow;
		}
	}
}


float3 SpherePathtracingWithDirectLight(float G, float Phi, float Sigma, float3 x, float3 w, out int bounces)
{
	bounces = 0;
	float3 A = 0;
	[loop]
	while (true) {
		bounces++;
		float t = abs(Sigma) <= 0.001 ? 10000 : -log(max(0.0000001, 1 - random())) / Sigma;
		float d = exitSphere(x, w);

		x += min(t, d) * w;

		bool scatters = true;
		if (t >= d)
		{
			float3 N = normalize(x);
			float F = ComputeFresnel(dot(-N, -w), 1.76);
			float3 T = refract(w, -N, 1.76);
			F = any(T) ? F : 1;
			if (random() < 1 - F)
			{
				w = T;
				return A;// +SampleSkybox(w);
			}
			else
			{
				w = reflect(w, -N);
				x *= 0.9999;
				scatters = false;
			}
		}

		if (scatters) {
			float r = max(0, 0.99 - length(x));
			//float r = min(1 - length(x), 500 / Sigma);
			float density = Sigma * r;

			float3 X, W;
			float3 ox, ow;
			float factor;
			// Using the Model
			if (!GenerateFullVariablesWithNewModel(G, Phi, w, density, ox, ow, factor, X, W))
				return A;

			// Using the generator
			//float accum, importance;
			//GenerateVariables(G, Phi, w, density, ox, ow, X, W, accum, importance);
			//factor = accum / importance;
			//if (random() < 1 - importance) // Absortion
			//	return A;

			//float n = s * 2;
			//float absorption = Phi == 1 ? 0 : 1 - 1.0 / (n * log(Phi));

			//float absorption = 1 - pow(Phi, int(xd * density / 0.7764803));

			//if (random() < 1 - importance) // Absortion
			//	return A;

			A += LightIntensity / (2 * pi) * exp(-(exitSphere(x + X * r, LightDirection)) * Sigma) * EvalPhase(G, W, LightDirection) * factor;
			//float accum = Phi == 1 ? n : Phi * pow(1 - Phi, n) / (1 - Phi);
			//I *= importance;

			x += ox * r;
			w = ow;
		}
	}
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
	w = randomDirection(-win);
	x = w;

	if (random() > Phi)  // absorption at first scatteing
		return false;

	//if (density < 100) // no infer position for small steps
	//{
	//	x = 0;
	//	return true;
	//}

	if (-log(1 - random()) / density >= 1) // single scattering
	{
		return true;
	}

	float mlogN = -0.590368 + 0.968372 * log(density * density + 5.02256);
	float vlogN = 0.34273;

	float n = max(2, (int)exp(gauss(mlogN, sqrt(vlogN))));

	if (random() >= pow(Phi, n))
		return false;

	float3 oB = abs(x.z) < .9999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 oT = normalize(cross(oB, x));
	oB = cross(oT, x);

	float randomRot = random() * 2 * pi;

	float gamma = pi * 0.5 * randomKum(AlphaFor(density, Phi), BetaFor(density, Phi));
	//float cosgamma = gamma;
	//float singamma = sqrt(max(0, 1 - cosgamma * cosgamma));
	float cosgamma = cos(gamma);
	float singamma = sin(gamma);// sqrt(max(0, 1 - cosgamma * cosgamma));

	w = singamma * cos(randomRot) * oB + singamma * sin(randomRot) * oT + cosgamma * x;
	return true;
}

float GenerateVariablesAnalytically2(float Phi, float density, float3 win, out float3 x, out float3 w)
{
	x = randomDirection(-win);
	w = x;

	float absorption = 1 - Phi;

	float prob = 1 - exp(-density);
	if (random() > prob) // single scattering
	{
		return absorption;
	}

	float mlogN = -0.590368 + 0.968372 * log(density * density + 5.02256);
	float vlogN = 0.34273;

	float logN = gauss(mlogN, sqrt(vlogN));

	absorption = (1 - pow(Phi, max(2, (int)exp(logN))));

	float3 oB = abs(x.z) < 1 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 oT = normalize(cross(oB, x));
	oB = cross(oT, x);

	float cosgamma = randomKum(AlphaFor(density, Phi), BetaFor(density, Phi));
	float randomRot = random() * 2 * pi;
	float singamma = sqrt(max(0, 1 - cosgamma * cosgamma));
	w = cosgamma * cos(randomRot) * oB + cosgamma * sin(randomRot) * oT + singamma * x;
	return absorption;
}

float3 IsotropicSpherePathtracing2(float G, float Phi, float Sigma, float3 x, float3 w, out int bounces)
{
	bounces = 0;
	float3 I = 1;
	[loop]
	while (true) {
		bounces++;
		float t = abs(Sigma) <= 0.001 ? 10000 : -log(max(0.0000001, 1 - random())) / Sigma;
		float d = exitSphere(x, w);

		x += min(t, d) * w;

		if (t >= d)
		{
			float3 N = normalize(x);
			float F = ComputeFresnel(dot(-N, -w), 1.76);
			float3 T = refract(w, -N, 1.76);
			if (any(T)) {
				w = T;
				return I * SampleSkybox(w) * (1 - F);
			}
			else
				w = reflect(w, -N);
		}

		float r = 1 - length(x);
		float density = Sigma * r;

		float3 ox, ow;
		float absorption = GenerateVariablesAnalytically2(Phi, density, w, ox, ow);
		I *= (1 - absorption);

		x += ox * r;
		w = ow;
	}
}

float3 IsotropicSpherePathtracing(float G, float Phi, float Sigma, float3 x, float3 w, out int bounces)
{
	bounces = 0;
	[loop]
	while (true) {
		bounces++;
		float t = abs(Sigma) <= 0.00001 ? 10000 : -log(max(0.0000001, 1 - random())) / Sigma;
		float d = exitSphere(x, w);

		x += min(t, d) * w;
		bool scatter = true;

		if (t >= d)
		{
			float3 N = normalize(x);
			float F = ComputeFresnel(dot(-N, -w), 1.76);
			float3 T = refract(w, -N, 1.76);
			F = any(T) ? F : 1;
			if (random() < 1 - F) {
				w = T;
				return SampleSkybox(w);
			}
			else
			{
				w = reflect(w, -N);
				x *= 0.9999; // avoid self hit
				scatter = false;
			}
		}

		if (scatter) {
			float r = max(0, 1 - length(x));
			float density = Sigma * r;

			float3 ox, ow;
			float accum, importance;
			float3 X, W;
			//GenerateVariables(G, Phi, w, density, ox, ow, X, W, accum, importance);

			//if (random() >= importance)
			//	return 0;

			if (!GenerateVariablesAnalytically(Phi, density, w, ox, ow))
				return 0;

			x += ox * r;
			w = ow;
		}
	}
}

//
//float GetInnerMaximalSphereRadius(float3 P, float3 D) {
//	float negc = 1 - dot(P, P);
//	float b = 2 * dot(P, D) + 2; // muladd better
//	return negc / b;
//}

#include "../CommonRT/CommonComplexity.h"

float3 filter(float3 color) {
	return color;

	float x = min(1, dot(color, 1) / 3.0f);
	if (sin(x * 70) > 0.95)
		return 0;

	return color;
	//return (color.x + color.y + color.z) / 3 > 0.5 ? 1 : 0;
}

[numthreads(CS_2D_GROUPSIZE, CS_2D_GROUPSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	Output.GetDimensions(dim.x, dim.y);

	if (!all(DTid.xy < dim))
		return;

	StartRandomSeedForRay(dim, 1, DTid.xy, 0, PassCount);

	float4 ndcP = float4(2 * (DTid.xy + 0.5) / dim - 1, 0, 1);
	ndcP.y *= -1;
	float4 ndcT = ndcP + float4(0, 0, 1, 0);

	float4 viewP = mul(ndcP, FromProjectionToWorld);
	viewP.xyz /= viewP.w;
	float4 viewT = mul(ndcT, FromProjectionToWorld);
	viewT.xyz /= viewT.w;

	float3 O = viewP.xyz;
	float3 D = normalize(viewT.xyz - viewP.xyz);

	float3 acc = 0;// SampleSkybox(D);

	int bounces = 0;

	float inT, outT;
	if (unitSphereIntersections(O, D, inT, outT))
	{
		O += D * inT; // move to sphere surface
		float3 N = normalize(O);
		float F = ComputeFresnel(dot(N, -D), 1 / 1.76);
		float3 T = refract(D, N, 1 / 1.76); // refraction index
		float3 R = reflect(D, N);

		if (random() < F)
			acc = SampleSkybox(R);
		else
		{
			D = T;

			int cmpSel = 3 * random();
			float3 filter = 0;
			filter[cmpSel] = 1;
			float g = dot(filter, G);
			float phi = dot(filter, Phi);
			float sigma = dot(filter, Sigma) * 0.5; // the 0.5 is to normalize the sphere height to 1.
			filter *= 3; // divided by 1/3
			//if (abs(O.x * 0.5 + 0.5 - PathtracingPercentage) <= 0.001)
			//	acc = float3(0, 0, 0);
			//else
			if (O.x * 0.5 + 0.5 < PathtracingPercentage)
				acc = Pathtrace(g, phi, sigma, O, D, bounces) * filter;
			//acc = PathtraceNoDirectLight(g, phi, sigma, O, D, bounces) * filter;
			else
#ifdef USE_DIRECT_LIGHT
				//acc = PathtraceNoDirectLight(g, phi, sigma, O, D, bounces) * filter;
				acc = SpherePathtracingWithDirectLight(g, phi, sigma, O, D, bounces) * filter;
#else
				if (abs(g) <= 0.001)
					acc = IsotropicSpherePathtracing(g, phi, sigma, O, D, bounces) * filter;
				else
					acc = SpherePathtracing(g, phi, sigma, O, D, bounces) * filter;
#endif
		}
	}

	acc = max(0, acc);

	if (abs(DTid.x - dim.x / 2) <= 1)
		acc = 0;// middle line

	bool debug = false;

	if (CountSteps > 0) //  debug complexity
	{
		Accumulation[DTid.xy] = (Accumulation[DTid.xy] * PassCount + bounces / 1000.0) / (PassCount + 1);
		Output[DTid.xy] = GetColor((int)(Accumulation[DTid.xy].x * 1000));
	}
	else {
		Accumulation[DTid.xy] += acc;
		Output[DTid.xy] = filter(Accumulation[DTid.xy] / (PassCount + 1));
	}
}