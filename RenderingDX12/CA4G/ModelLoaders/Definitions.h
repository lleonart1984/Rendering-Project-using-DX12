#pragma once

#include "..\ca4g.h"


#define vertex_layout(layout) static std::initializer_list<VertexElement> Layout() \
							{ static std::initializer_list<VertexElement> result layout; return result; }


// Default vertex definition for Scene vertex
struct SCENE_VERTEX {
	float3 Position;
	float3 Normal;
	float2 Coordinates;
	float3 Tangent;
	float3 Binormal;

	static std::initializer_list<VertexElement> Layout() {
		static std::initializer_list<VertexElement> result{
			VertexElement(VertexElementType_Float, 3, "POSITION"),
			VertexElement(VertexElementType_Float, 3, "NORMAL"),
			VertexElement(VertexElementType_Float, 2, "TEXCOORD"),
			VertexElement(VertexElementType_Float, 3, "TANGENT"),
			VertexElement(VertexElementType_Float, 3, "BINORMAL")
		};

		return result;
	}
};

struct SCENE_MATERIAL {
	float3 Diffuse;
	float3 Specular;
	float SpecularSharpness;
	int Diffuse_Map;
	int Specular_Map;
	int Bump_Map;
	int Mask_Map;
	float3 Emissive;
	float4 Roulette; // x-diffuse, y-mirror, z-fresnell, w-reflection index
	SCENE_MATERIAL() {
		Diffuse = float3(1, 1, 1);
		Specular = float3(0, 0, 0);
		SpecularSharpness = 1;
		Diffuse_Map = -1;
		Specular_Map = -1;
		Bump_Map = -1;
		Mask_Map = -1;
		Emissive = float3(0, 0, 0);
		Roulette = float4(1, 0, 0, 1);
	}
};

struct SCENE_OBJECT {
	int startVertex;
	int vertexesCount;
	SCENE_MATERIAL* Material;
	float4x4* Transform;
};
