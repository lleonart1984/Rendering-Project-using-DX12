// Material info
struct Material
{
	float3 Diffuse;
	float3 Specular;
	float SpecularSharpness;
	int4 Texture_Index; // X - diffuse map, Y - Specular map, Z - Bump Map, W - Mask Map
	float3 Emissive; // Emissive component for lights
	float4 Roulette; // X - Diffuse ammount, Y - Mirror scattering, Z - Fresnell scattering, W - Refraction Index (if fresnel)
};

// Vertex Data
struct Vertex
{
	// Position
	float3 P;
	// Normal
	float3 N;
	// Texture coordinates
	float2 C;
	// Tangent Vector
	float3 T;
	// Binormal Vector
	float3 B;
};

// Photon Data
struct Photon {
	float3 Direction;
	float3 Intensity;
	float3 Position;
	float Radius;
};

#define IS_TRIANGLE_MASK 1
#define IS_PHOTON_MASK 2

// Photon AABB created to construct future photon map (via raytracing acceleration data structure)
struct AABB {
	float3 Minimum;
	float3 Maximum;
};

// Top level structure with the scene
RaytracingAccelerationStructure Scene : register(t0, space0);
StructuredBuffer<Vertex> vertices		: register(t1);
StructuredBuffer<Material> materials	: register(t2);

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(t3);
Texture2D<float3> Normals				: register(t4);
Texture2D<float2> Coordinates			: register(t5);
Texture2D<int> MaterialIndices			: register(t6);

// Textures
Texture2D<float4> Textures[500]			: register(t7);

// Photon Map binding objects
RWStructuredBuffer<AABB> PhotonAABBs	: register(u0);
RWStructuredBuffer<Photon> Photons		: register(u1);
RWStructuredBuffer<int> Malloc			: register(u2);

SamplerState gSmp : register(s0);

// Global constant buffer with view to world transform matrix
cbuffer Camera : register(b0) {
	// Light Space (View) to world space transform
	row_major matrix ViewToWorld;
}

cbuffer Lighting : register(b1) {
	float3 LightPosition;
	float3 LightIntensity;
}

struct ObjInfo {
	int TriangleOffset;
	int MaterialIndex;
};
// Locals for hit groups
ConstantBuffer<ObjInfo> objectInfo : register(b2);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload
{
	float3 color;
	int bounce;
};

static float pi = 3.1415926;
static uint rng_state;
uint rand_xorshift()
{
	// Xorshift algorithm from George Marsaglia's paper
	rng_state ^= (rng_state << 13);
	rng_state ^= (rng_state >> 17);
	rng_state ^= (rng_state << 5);
	return rng_state;
}
float random()
{
	return rand_xorshift() * (1.0 / 4294967296.0);
}
float3 randomDirection()
{
	float r1 = random();
	float r2 = random() * 2 - 1;
	float x = cos(2.0 * pi * r1) * sqrt(1.0 - r2 * r2);
	float y = sin(2.0 * pi * r1) * sqrt(1.0 - r2 * r2);
	float z = r2;
	return float3(x, y, z);
}

void ComputeFresnel(float3 dir, float3 faceNormal, float ratio, out float reflection, out float refraction)
{
	float f = ((1.0 - ratio) * (1.0 - ratio)) / ((1.0 + ratio) * (1.0 + ratio));

	float Ratio = f + (1.0 - f) * pow((1.0 + dot(dir, faceNormal)), 5);

	reflection = min(1, Ratio);
	refraction = max(0, 1 - reflection);
}

Vertex Transform(Vertex surfel, float4x3 transform) {
	Vertex v = {
		mul(float4(surfel.P, 1), transform),
		mul(float4(surfel.N, 0), transform),
		surfel.C,
		mul(float4(surfel.T, 0), transform),
		mul(float4(surfel.B, 0), transform) };
	return v;
}

void PhotonScatter(float3 L, Vertex surfel, Material material, out float3 ratio, out float3 direction) {
	ratio = 1;
	direction = float3(0, 0, 0);

	// Transparent hit
	if (material.Roulette.w == 0)
	{
		direction = -L; // continue without interaction
		return;
	}

	float scatteringSelection = random();

	bool entering = dot(L, surfel.N) > 0;

	float3 facedNormal = entering ? surfel.N : -surfel.N;

	// Diffuse photon scattering
	if (scatteringSelection < material.Roulette.x)
	{
		direction = randomDirection();
		if (dot(direction, facedNormal) < 0)
			direction *= -1; // invert direction to head correct hemisphere (facedNormal)
		ratio = material.Diffuse * dot(L, facedNormal); // lambert factor only affects diffuse scattering
		// uniform hemisphere distribution pdf would be 1/pi. but diffuse normalization is 1/pi. so,
		// they are canceled
		return;
	}

	// Mirror photon scattering
	if (scatteringSelection < material.Roulette.x + material.Roulette.y)
	{
		direction = reflect(-L, facedNormal);
		// impulse bounce is not affected by lambert law (cosine factor)
		return;
	}

	// Fresnel scattering
	if (scatteringSelection < material.Roulette.x + material.Roulette.y + material.Roulette.z)
	{
		float reflectionIndex, refractionIndex;
		float eta = entering ? 1 / material.Roulette.w : material.Roulette.w;
		ComputeFresnel(-L, facedNormal, eta,
			reflectionIndex, refractionIndex);
		float3 reflectionDir = reflect(-L, facedNormal);
		float3 refractionDir = refract(-L, facedNormal, eta);
		if (!any(refractionDir))
		{
			reflectionIndex = 1;
			refractionIndex = 0; // total internal reflection
		}
		float fresnelSelection = random();
		direction = (fresnelSelection < reflectionIndex) ?
			// Select to reflect
			reflectionDir :
			// Select to refract
			refractionDir;
		// impulse bounce is not affected by lambert law (cosine factor)
		return;
	}
	// Photon absortion
}

void AugmentHitInfoWithTextureMapping(bool onlyMaterial, inout Vertex surfel, inout Material material) {
	float4 DiffTex = material.Texture_Index.x >= 0 ? Textures[material.Texture_Index.x].SampleGrad(gSmp, surfel.C, 0, 0) : float4(1, 1, 1, 1);
	float3 SpecularTex = material.Texture_Index.y >= 0 ? Textures[material.Texture_Index.y].SampleGrad(gSmp, surfel.C, 0, 0) : material.Specular;
	float3 BumpTex = material.Texture_Index.z >= 0 ? Textures[material.Texture_Index.z].SampleGrad(gSmp, surfel.C, 0, 0) : float3(0.5, 0.5, 1);
	float3 MaskTex = material.Texture_Index.w >= 0 ? Textures[material.Texture_Index.w].SampleGrad(gSmp, surfel.C, 0, 0) : 1;

	if (!onlyMaterial) {
		float3x3 TangentToWorld = { surfel.T, surfel.B, surfel.N };
		// Change normal according to bump map
		surfel.N = normalize(mul(BumpTex * 2 - 1, TangentToWorld));
	}

	material.Diffuse *= DiffTex * MaskTex.x; // set transparent if necessary.
	material.Specular.xyz = max(material.Specular.xyz, SpecularTex);
}

// Photon trace stage
// Photon rays has the origin in light source and select a random direction (point source)
[shader("raygeneration")]
void PTMainRays() {
	uint2 raysIndex = DispatchRaysIndex();
	uint2 raysDimensions = DispatchRaysDimensions();

	// Initialize random iterator using rays index as seed
	rng_state = raysIndex.x + raysIndex.y*raysDimensions.x;
	// avoid strong correlation at begining
	random(); random(); random();
	//random(); random(); random();

	RayDesc ray;
	ray.TMin = 0.001;
	ray.TMax = 10000.0;
	// photons travel with a piece of intensity. This could produce numerical problems and it is better to add entire intensity
	// and then divide by the number of photons.
	// Box distribution normalization factor
	float nfactor = length(float3(2 * raysIndex / (float2)raysDimensions - 1, 1));
	RayPayload payload = { LightIntensity * 100000 / (nfactor * raysDimensions.x * raysDimensions.y), 1 };

	float3 P = Positions[raysIndex];
	float3 N = Normals[raysIndex];
	float2 C = Coordinates[raysIndex];
	Material material = materials[MaterialIndices[raysIndex]];
	if (!any(P)) // force miss execution
	{
		return;
	}

	// Move to world space
	P = mul(float4(P, 1), ViewToWorld).xyz;
	N = mul(float4(N, 0), ViewToWorld).xyz;
	float3 L = normalize(LightPosition - P); // V is really L in this case, since the "viewer" is positioned in light position to trace rays

	Vertex surfel = {
		P,
		N,
		C,
		float3(0,0,0),
		float3(0,0,0)
	};

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentHitInfoWithTextureMapping(true, surfel, material);

	float3 direction, ratio;
	PhotonScatter(L, surfel, material, ratio, direction);

	if (any(direction))
	{
		ray.Origin = surfel.P + sign(dot(direction, surfel.N))*surfel.N*0.001;
		ray.Direction = direction;
		payload.color *= ratio;
		// Trace the ray.
		// Set the ray's extents.
		if (dot(payload.color, payload.color) > 0.0001)
			TraceRay(Scene, RAY_FLAG_NONE, IS_TRIANGLE_MASK, 0, 1, 0, ray, payload); // Will be used with Photon scattering function
	}
}

[shader("miss")]
void PhotonMiss(inout RayPayload payload) {
	// Do nothing... farewell my photon friend...
}

void GetHitInfo(in MyAttributes attr, out Vertex surfel, out Material material)
{
	float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

	uint triangleIndex = objectInfo.TriangleOffset + PrimitiveIndex();
	uint materialIndex = objectInfo.MaterialIndex;

	Vertex v1 = vertices[triangleIndex * 3 + 0];
	Vertex v2 = vertices[triangleIndex * 3 + 1];
	Vertex v3 = vertices[triangleIndex * 3 + 2];
	Vertex s = {
		v1.P * barycentrics.x + v2.P * barycentrics.y + v3.P * barycentrics.z,
		v1.N * barycentrics.x + v2.N * barycentrics.y + v3.N * barycentrics.z,
		v1.C * barycentrics.x + v2.C * barycentrics.y + v3.C * barycentrics.z,
		v1.T * barycentrics.x + v2.T * barycentrics.y + v3.T * barycentrics.z,
		v1.B * barycentrics.x + v2.B * barycentrics.y + v3.B * barycentrics.z
	};

	surfel = Transform(s, ObjectToWorld4x3());

	material = materials[materialIndex];

	AugmentHitInfoWithTextureMapping(false, surfel, material);
}

[shader("closesthit")]
void PhotonScattering(inout RayPayload payload, in MyAttributes attr)
{
	Vertex surfel;
	Material material;
	GetHitInfo(attr, surfel, material);

	float3 V = -normalize(WorldRayDirection());

	if (material.Roulette.x > 0) // Material has some diffuse component
	{ // Store the photon in the photon map
		int photonIndexInBuffer;
		InterlockedAdd(Malloc[0], 1, photonIndexInBuffer);
		float radius = 0.01;
		Photon p = {
			surfel.P,
			WorldRayDirection(), // photon direction
			payload.color / (4 * radius * radius), // photon intensity in a specific area
			radius // photon radius
		};

		AABB box = {
			surfel.P - radius,
			surfel.P + radius
		};

		PhotonAABBs[photonIndexInBuffer] = box;
		Photons[photonIndexInBuffer] = p;
	}

	//if (false)
	if (payload.bounce > 0) // Photon can bounce one more time
	{
		float scatteringSelection = random();

		RayPayload newPhotonPayload = { float3(0,0,0), payload.bounce - 1 };
		RayDesc newPhotonRay;
		newPhotonRay.Direction = float3(0, 0, 0);
		newPhotonRay.Origin = surfel.P;
		newPhotonRay.TMin = 0.001;
		newPhotonRay.TMax = 10000.0;

		float3 facedNormal = dot(V, surfel.N) > 0 ? surfel.N : -surfel.N;

		if (scatteringSelection < material.Roulette.x) // Diffuse photon scattering
		{
			//// THIS SHOULD BE COMMENTED TO BOUNCE ONLY IN SPECULAR (MIRROR AND FRESNEL) SCATTERED MATERIALS
			//// ONLY FOR PERFORMANCE PURPOSES

			//newPhotonRay.Direction = randomDirection();
			//if (dot(newPhotonRay.Direction, facedNormal) < 0)
			//	newPhotonRay.Direction *= -1; // invert direction to head correct hemisphere (facedNormal)
			//newPhotonPayload.color = payload.color * material.Diffuse * dot(V, facedNormal);
		}
		else if (scatteringSelection < material.Roulette.x + material.Roulette.y) // Mirror photon scattering
		{
			newPhotonRay.Direction = reflect(-V, facedNormal);
			newPhotonPayload.color = payload.color; // impulse bounce is not affected by lambert law (cosine factor)
		}
		else if (scatteringSelection < material.Roulette.x + material.Roulette.y + material.Roulette.z) // Fresnel scattering
		{
			bool entering = dot(V, surfel.N) > 0;
			float reflectionIndex, refractionIndex;
			float eta = entering ? 1 / material.Roulette.w : material.Roulette.w;
			float3 fN = entering ? surfel.N : -surfel.N;
			ComputeFresnel(-V, fN, eta,
				reflectionIndex, refractionIndex);
			float3 reflectionDir = reflect(-V, fN);
			float3 refractionDir = refract(-V, fN, eta);
			if (!any(refractionDir))
			{
				reflectionIndex = 1;
				refractionIndex = 0; // total internal reflection
			}
			float fresnelSelection = random();
			newPhotonRay.Direction = (fresnelSelection < reflectionIndex) ?
				// Select to reflect
				reflectionDir :
				// Select to refract
				refractionDir;
			newPhotonPayload.color = payload.color; // impulse bounce is not affected by lambert law (cosine factor)
		}
		else { // Photon absortion
		}

		if (newPhotonPayload.color.x + newPhotonPayload.color.y + newPhotonPayload.color.z > 0.001) // only continue with no-obscure photons
		{
			newPhotonRay.Origin += sign(dot(newPhotonRay.Direction, facedNormal))*0.01*facedNormal; // avoid self shadowing
			TraceRay(Scene, RAY_FLAG_FORCE_OPAQUE, 0xFF, 0, 1, 0, newPhotonRay, newPhotonPayload); // Will be used with Photon scattering function
		}
	}
}
