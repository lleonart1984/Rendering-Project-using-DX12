#pragma once
#include "Definitions.h"
#include "Tokenizer.h"

class BaseLoader {

public:
	SCENE_VERTEX* VerticesData = nullptr;
	int* ObjectsId = nullptr;
	int VerticesDataLength;

	int* MaterialIndices = nullptr;
	float4x4* TransformsData = nullptr;
	int ObjectsLength;
	SCENE_OBJECT* Objects = nullptr;

	list<TextureData*> Textures = { };
	list<string> textureNames = { };
	list<SCENE_MATERIAL> MaterialsData = { };
	list<string> materialNames = { };

	float3 Minim;
	float3 Maxim;

	virtual void Load(const char *fileName) = 0;

	void ComputeTangents()
	{
		for (int i = 0; i < VerticesDataLength; i += 3)
		{
			//Getting vertex from indices
			SCENE_VERTEX &vertex1 = VerticesData[i];
			SCENE_VERTEX &vertex2 = VerticesData[i + 1];
			SCENE_VERTEX &vertex3 = VerticesData[i + 2];

			//making 
			float3 edge_1_2 = vertex2.Position - vertex1.Position;
			float3 edge_1_3 = vertex3.Position - vertex1.Position;

			float2 coordinate_2_1 = vertex2.Coordinates - vertex1.Coordinates;
			float2 coordinate_3_1 = vertex3.Coordinates - vertex1.Coordinates;

			float alpha = 1.0f / (coordinate_2_1.x * coordinate_3_1.y - coordinate_2_1.y * coordinate_3_1.x);

			vertex1.Tangent = vertex2.Tangent = vertex3.Tangent = float3(
				((edge_1_2.x * coordinate_3_1.y) - (edge_1_3.x * coordinate_2_1.y)) * alpha,
				((edge_1_2.y * coordinate_3_1.y) - (edge_1_3.y * coordinate_2_1.y)) * alpha,
				((edge_1_2.z * coordinate_3_1.y) - (edge_1_3.z * coordinate_2_1.y)) * alpha
			);

			vertex1.Binormal = vertex2.Binormal = vertex3.Binormal = float3(
				((edge_1_2.x * coordinate_3_1.x) - (edge_1_3.x * coordinate_2_1.x)) * alpha,
				((edge_1_2.y * coordinate_3_1.x) - (edge_1_3.y * coordinate_2_1.x)) * alpha,
				((edge_1_2.z * coordinate_3_1.x) - (edge_1_3.z * coordinate_2_1.x)) * alpha
			);
		}

	}

protected:

	void addMaterial(string name, SCENE_MATERIAL material) {
		materialNames.add(name);
		MaterialsData.add(material);
	}

	int resolveTexture(string subdir, string fileName) {
		for (int i = 0; i < textureNames.size(); i++)
			if (textureNames[i] == fileName)
				return i;

		string full = subdir;
		full += fileName;

		TextureData* data = TextureData::LoadFromFile(full.c_str());
		
		if (data == nullptr) // Image couldn't be loaded
		{
			MessageBoxA(hWnd, full.c_str(), "Couldnt load texture", 0);
			return -1;
		}
		textureNames.add(fileName);
		Textures.add(data);
		return Textures.size() - 1;
	}

	int getMaterialIndex(string materialName) {
		for (int i = 0; i < materialNames.size(); i++)
			if (materialNames[i] == materialName)
				return i;
		materialName = "";
		return 0;
	}
};