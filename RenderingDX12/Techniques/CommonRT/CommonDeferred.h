/// A common header for libraries will use a GBuffer for deferred computation
/// Requires:
/// A position texture at GBUFFER_POSITIONS_REG register
/// A normal texture at GBUFFER_NORMALS_REG register
/// A coordinates texture at GBUFFER_COORDINATES_REG register
/// A material indices texture at GBUFFER_MATERIALS_REG register
/// A constant buffer with matrix to transform from GBuffer view to world at VIEWTOWORLD_CB_REG register
/// Provides a function to determine a primary intersection with scene geometry in deferred mode

#include "CommonRTScenes.h"

// GBuffer Used for primary rays (from light in photon trace and from viewer in raytrace)
Texture2D<float3> Positions				: register(GBUFFER_POSITIONS_REG);
Texture2D<float3> Normals				: register(GBUFFER_NORMALS_REG);
Texture2D<float2> Coordinates			: register(GBUFFER_COORDINATES_REG);
Texture2D<int> MaterialIndices			: register(GBUFFER_MATERIALS_REG);

// Global constant buffer with view to world transform matrix
cbuffer ViewToWorldTransform : register(VIEWTOWORLD_CB_REG) {
	row_major matrix ViewToWorld;
}

/*bool GetPrimaryIntersection(uint2 screenCoordinates, out float3 V, out Vertex surfel, out Material material) {
	
	V = float3(0, 0, 0);

	surfel.P = Positions[screenCoordinates];
	surfel.N = Normals[screenCoordinates];
	surfel.C = Coordinates[screenCoordinates];
	surfel.T = float3(1, 0, 0);
	surfel.B = cross(surfel.N, surfel.T);

	material = materials[MaterialIndices[screenCoordinates]];

	if (!any(surfel.P)) // force miss execution
	{
		return false;
	}

	V = mul(float4(-normalize(surfel.P), 0), ViewToWorld).xyz;

	surfel = Transform(surfel, ViewToWorld);

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material);

	return true;
}*/


bool GetPrimaryIntersection(uint2 screenCoordinates, float2 coordinates, out float3 V, out Vertex surfel, out Material material) {
	bool valid = any(surfel.P = Positions[screenCoordinates]);
	float d = length(surfel.P);

	//surfel.N = Normals[screenCoordinates];
	surfel.N = Normals.SampleGrad(gSmp, coordinates, 0.001, 0.001);// [screenCoordinates];
	//surfel.C = Coordinates[screenCoordinates];
	surfel.C = Coordinates.SampleGrad(gSmp, coordinates, 0.001, 0.001);// [screenCoordinates];
	surfel.T = float3(1, 0, 0);
	surfel.B = cross(surfel.N, surfel.T);


	V = -surfel.P / d;

	material = materials[MaterialIndices[screenCoordinates]];

	V = mul(float4(V, 0), ViewToWorld).xyz;

	surfel = Transform(surfel, ViewToWorld);

	// only update material, Normal is affected with bump map from gbuffer construction
	AugmentMaterialWithTextureMapping(surfel, material);

	return valid;
}