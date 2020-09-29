

float random();

float3 randomHSDirection(float3 N, out float NdotD)
{
	float r1 = random();
	float r2 = random() * 2 - 1;
	float sqrR2 = r2 * r2;
	float two_pi_by_r1 = two_pi * r1;
	float sqrt_of_one_minus_sqrR2 = sqrt(1.0 - sqrR2);
	float x = cos(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float y = sin(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float z = r2;
	float3 d = float3(x, y, z);
	NdotD = dot(N, d);
	d *= (NdotD < 0 ? -1 : 1);
	NdotD *= NdotD < 0 ? -1 : 1;
	return d;
}

void CreateOrthonormalBasis2(float3 D, out float3 B, out float3 T) {
	float3 other = abs(D.z) >= 0.999 ? float3(1, 0, 0) : float3(0, 0, 1);
	B = normalize(cross(other, D));
	T = normalize(cross(D, B));
}


float3 randomDirection(float3 D) {
	float r1 = random();
	float r2 = random() * 2 - 1;
	float sqrR2 = r2 * r2;
	float two_pi_by_r1 = two_pi * r1;
	float sqrt_of_one_minus_sqrR2 = sqrt(1.0 - sqrR2);
	float x = cos(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float y = sin(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float z = r2;

	float3 t0, t1;
	CreateOrthonormalBasis2(D, t0, t1);

	return t0 * x + t1 * y + D * z;
}

float3 randomDirectionWrong()
{
	while (true)
	{
		float3 x = float3(random(), random(), random()) * 2 - 1;
		float lsqr = dot(x, x);
		float s;
		if (lsqr > 0.01 && lsqr < 1)
			return normalize(x);// randomHSDirection(x / sqrt(lsqr), s);
	}
	/*
	float r1 = random();
	float r2 = random() * 2 - 1;
	float sqrR2 = r2 * r2;
	float two_pi_by_r1 = two_pi * r1;
	float sqrt_of_one_minus_sqrR2 = sqrt(max(0, 1.0 - sqrR2));
	float x = cos(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float y = sin(two_pi_by_r1) * sqrt_of_one_minus_sqrR2;
	float z = r2;
	return float3(x, y, z);*/
}

float gauss(float mu = 0, float sigma = 1) {
	float u1 = 1.0 - random(); //uniform(0,1] random doubles
	float u2 = 1.0 - random();
	float randStdNormal = sqrt(-2.0 * log(max(0.0000000001, u1))) *
		sin(2.0 * pi * u2); //random normal(0,1)
	return mu + sigma * randStdNormal; //random normal(mean,stdDev^2)
}

float randomStdNormal() {
	float u1 = 1.0 - random(); //uniform(0,1] random doubles
	float u2 = 1.0 - random();
	return sqrt(-2.0 * log(max(0.0000000001, u1))) *
		sin(2.0 * pi * u2); //random normal(0,1)
}

float3 randomStdNormal3() {
	float3 u1 = 1.0 - float3(random(), random(), random()); //uniform(0,1] random doubles
	float3 u2 = 1.0 - float3(random(), random(), random());
	return sqrt(-2.0 * log(max(0.0000000001, u1))) *
		sin(2.0 * pi * u2); //random normal(0,1)
}

float2 randomStdNormal2() {
	float2 u1 = 1.0 - float2(random(), random()); //uniform(0,1] random doubles
	float2 u2 = 1.0 - float2(random(), random());
	return sqrt(-2.0 * log(max(0.0000000001, u1))) *
		sin(2.0 * pi * u2); //random normal(0,1)
}

float4 randomStdNormal4() {
	float4 u1 = 1.0 - float4(random(), random(), random(), random()); //uniform(0,1] random doubles
	float4 u2 = 1.0 - float4(random(), random(), random(), random());
	return sqrt(-2.0 * log(max(0.0000000001, u1))) *
		sin(2.0 * pi * u2); //random normal(0,1)
}