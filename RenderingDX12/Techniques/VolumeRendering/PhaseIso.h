

float EvalPhase(float3 V, float3 L) {
	return 0.25 / pi;
}

void GeneratePhase(float3 V, out float3 L, out float pdf) {
	L = randomDirection();
	pdf = 0.25 / pi;
}