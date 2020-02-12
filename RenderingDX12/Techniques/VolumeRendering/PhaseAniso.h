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

float invertcdf(float xi) {
	float t = (one_minus_g2) / (1.0f - g + 2.0f * g * xi);
	return one_over_2g * (one_plus_g2 - t * t);
}
float calcpdf(float costheta) {
	return 0.25 * one_minus_g2 / (pi * pow(one_plus_g2 - 2.0f * g * costheta,
		1.5f));
}

void CreateOrthonormalBasis(float3 D, out float3 B, out float3 T) {
	float3 other = abs(D.z) == 1 ? float3(1, 0, 0) : float3(0, 0, 1);
	B = normalize(cross(other, D));
	T = normalize(cross(D, B));
}

//float random();

void GeneratePhase(float3 D, out float3 L, out float pdf) {
	float phi = random() * 2 * 3.14159;
	float cosTheta = invertcdf(random());
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	float3 t0, t1;
	CreateOrthonormalBasis(D, t0, t1);

	L = sinTheta * sin(phi) * t0 + sinTheta * cos(phi) * t1 +
		cosTheta * D;

	pdf = calcpdf(cosTheta);
}