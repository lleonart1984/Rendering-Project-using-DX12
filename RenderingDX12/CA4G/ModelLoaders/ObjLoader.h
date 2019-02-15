#pragma once

#include "BaseLoader.h"

class ObjLoader : public BaseLoader {
public:

	void importMtlFile(string subdir, string fileName)
	{
		string file = subdir;
		file += fileName;

		FILE* f;
		if (fopen_s(&f, file.c_str(), "r"))
			return;
		Tokenizer t(f);
		while (!t.isEof())
		{
			if (t.match("newmtl "))
			{
				addMaterial(t.readToEndOfLine(), SCENE_MATERIAL());
				continue;
			}

			if (t.match("Kd "))
			{
				float r, g, b;
				t.readFloatToken(r);
				t.readFloatToken(g);
				t.readFloatToken(b);
				MaterialsData.last().Diffuse = float3(r, g, b);
				t.skipCurrentLine();
				continue;
			}

			if (t.match("Ks "))
			{
				float r, g, b;
				t.readFloatToken(r);
				t.readFloatToken(g);
				t.readFloatToken(b);
				MaterialsData.last().Specular = float3(r, g, b);
				t.skipCurrentLine();
				continue;
			}

			if (t.match("Tr "))
			{
				float r, g, b;
				t.readFloatToken(r);
				//t.readFloatToken(g);
				//t.readFloatToken(b);
				MaterialsData.last().Roulette.x = (1 - r);
				MaterialsData.last().Roulette.y = r;
				t.skipCurrentLine();
				continue;
			}

			/*if (t.match("Tf "))
			{
			float r, g, b;
			t.readFloatToken(r);
			t.readFloatToken(g);
			t.readFloatToken(b);
			activeMaterial->Roulette.y = (r + g + b) / 3;
			t.skipCurrentLine();
			continue;
			}*/

			if (t.match("Ni "))
			{
				float ni;
				t.readFloatToken(ni);
				MaterialsData.last().Roulette.w = ni;
				t.skipCurrentLine();
				continue;
			}

			if (t.match("Ns "))
			{
				float power;
				t.readFloatToken(power);
				MaterialsData.last().SpecularSharpness = power;
				t.skipCurrentLine();
				continue;
			}

			if (t.match("map_Kd "))
			{
				string textureName = t.readToEndOfLine();
				MaterialsData.last().Diffuse_Map = resolveTexture(subdir, textureName);
				continue;
			}

			if (t.match("map_Ks "))
			{
				string textureName = t.readToEndOfLine();
				MaterialsData.last().Specular_Map = resolveTexture(subdir, textureName);
				continue;
			}

			if (t.match("map_bump "))
			{
				string textureName = t.readToEndOfLine();
				MaterialsData.last().Bump_Map = resolveTexture(subdir, textureName);
				continue;
			}

			if (t.match("map_d "))
			{
				string textureName = t.readToEndOfLine();
				MaterialsData.last().Mask_Map = resolveTexture(subdir, textureName);
				continue;
			}

			t.skipCurrentLine();
		}
		fclose(f);
	}

	void addLineIndex(list<int> &indices, int index, int pos, int total) {
		if (index <= 0)
			index = total + index;

		if (pos <= 1)
			indices.add(index);
		else
		{
			indices.add(indices[indices.size() - 1]);
			indices.add(index);
		}
	}

	void ReadLineIndices(Tokenizer &t, list<int> &posIndices, list<int> &texIndices, list<int> &norIndices, int vCount, int vnCount, int vtCount)
	{
		int indexRead = 0;
		int pos = 0;
		int type = 0;

		bool p = false;
		bool n = false;
		bool te = false;
		while (!t.isEol())
		{
			int indexRead;
			if (t.readIntegerToken(indexRead))
			{
				switch (type)
				{
				case 0: addLineIndex(posIndices, (vCount + indexRead + 1) % (vCount + 1), pos, vCount);
					p = true;
					break;
				case 1: addLineIndex(texIndices, (vtCount + indexRead + 1) % (vtCount + 1), pos, vtCount);
					te = true;
					break;
				case 2: addLineIndex(norIndices, (vnCount + indexRead + 1) % (vnCount + 1), pos, vnCount);
					n = true;
					break;
				}
			}
			else
			{
				int a = 4;
				a = a + 5;
			}
			if (t.isEol())
			{
				if (p) {
					if (!n)
						addLineIndex(norIndices, 0, pos, vnCount);
					if (!te)
						addLineIndex(texIndices, 0, pos, vtCount);
					n = false;
					te = false;
					p = false;
				}
				t.skipCurrentLine();
				return;
			}
			if (t.matchSymbol('/'))
			{
				type++;
			}
			else
				if (t.matchSymbol(' '))
				{
					if (p) {
						if (!n)
							addLineIndex(norIndices, 0, pos, vnCount);
						if (!te)
							addLineIndex(texIndices, 0, pos, vtCount);
						n = false;
						te = false;
						p = false;
					}
					pos++;
					type = 0;
				}
				else
				{
					if (p) {
						if (!n)
							addLineIndex(norIndices, 0, pos, vnCount);
						if (!te)
							addLineIndex(texIndices, 0, pos, vtCount);
						n = false;
						te = false;
						p = false;
					}
					t.skipCurrentLine();
					return;
				}
		}
		if (p) {
			if (!n)
				addLineIndex(norIndices, 0, pos, vnCount);
			if (!te)
				addLineIndex(texIndices, 0, pos, vtCount);
		}
	}

	void addIndex(list<int> &indices, int index, int pos) {
		if (pos <= 2)
			indices.add(index);
		else
		{
			indices.add(indices[indices.size() - pos]); // vertex 0
			indices.add(indices[indices.size() - 1 - 1]);
			indices.add(index);
		}
	}
	
	void ReadFaceIndices(Tokenizer &t, list<int> &posIndices, list<int> &texIndices, list<int> &norIndices, int vCount, int vnCount, int vtCount)
	{
		int indexRead = 0;
		int pos = 0;
		int type = 0;

		bool p = false;
		bool n = false;
		bool te = false;
		while (!t.isEol())
		{
			int indexRead;
			if (t.readIntegerToken(indexRead))
			{
				switch (type)
				{
				case 0: addIndex(posIndices, (vCount + indexRead + 1) % (vCount + 1), pos);
					p = true;
					break;
				case 1: addIndex(texIndices, (vtCount + indexRead + 1) % (vtCount + 1), pos);
					te = true;
					break;
				case 2: addIndex(norIndices, (vnCount + indexRead + 1) % (vnCount + 1), pos);
					n = true;
					break;
				}
			}
			if (t.isEol())
			{
				if (p) {
					if (!n)
						addIndex(norIndices, 0, pos);
					if (!te)
						addIndex(texIndices, 0, pos);
					n = false;
					te = false;
					p = false;
				}
				t.skipCurrentLine();
				return;
			}
			if (t.matchSymbol('/'))
			{
				type++;
			}
			else
				if (t.matchSymbol(' '))
				{
					if (p) {
						if (!n)
							addIndex(norIndices, 0, pos);
						if (!te)
							addIndex(texIndices, 0, pos);
						n = false;
						te = false;
						p = false;
					}
					pos++;
					type = 0;
				}
				else
				{
					if (p) {
						if (!n)
							addIndex(norIndices, 0, pos);
						if (!te)
							addIndex(texIndices, 0, pos);
						n = false;
						te = false;
						p = false;
					}
					t.skipCurrentLine();
					return;
				}
		}
		if (p) {
			if (!n)
				addIndex(norIndices, 0, pos);
			if (!te)
				addIndex(texIndices, 0, pos);
		}
	}

	void Load(const char* filePath)
	{
		list<float3> positions;
		list<float3> normals;
		list<float2> texcoords;

		list<int> positionIndices;
		list<int> textureIndices;
		list<int> normalIndices;

		list<int> lpositionIndices;
		list<int> ltextureIndices;
		list<int> lnormalIndices;

		int offsetPos = 0;
		int offsetNor = 0;
		int offsetCoo = 0;

		string full(filePath);

		string subDir = full.substr(0, full.find_last_of('\\') + 1);
		string name = full.substr(full.find_last_of('\\') + 1);

		FILE* stream;

		Minim = float3(100000, 100000, 100000);
		Maxim = float3(-100000, -100000, -100000);

		errno_t err;
		if (err = fopen_s(&stream, filePath, "r"))
		{
			return;
		}

		list<int> groups;
		list<string> usedMaterials;

		Tokenizer t(stream);
		static int facecount = 0;

		while (!t.isEof())
		{
			if (t.match("v "))
			{
				float3 pos;
				t.readFloatToken(pos.x);
				t.readFloatToken(pos.y);
				t.readFloatToken(pos.z);

				Minim = minf(Minim, pos);
				Maxim = maxf(Maxim, pos);

				positions.add(pos);
				t.skipCurrentLine();
			}
			else
				if (t.match("vn ")) {
					float3 nor;
					t.readFloatToken(nor.x);
					t.readFloatToken(nor.y);
					t.readFloatToken(nor.z);
					normals.add(nor);
					t.skipCurrentLine();
				}
				else
					if (t.match("vt ")) {
						float2 coord;
						t.readFloatToken(coord.x);
						t.readFloatToken(coord.y);
						float z;
						t.readFloatToken(z);
						texcoords.add(coord);
						t.skipCurrentLine();
					}
					else
						if (t.match("l "))
						{
							ReadLineIndices(t, lpositionIndices, ltextureIndices, lnormalIndices, positions.size(), normals.size(), texcoords.size());
						}
						else
							if (t.match("f "))
							{
								ReadFaceIndices(t, positionIndices, textureIndices, normalIndices, positions.size(), normals.size(), texcoords.size());
							}
							else
								if (t.match("usemtl "))
								{
									string materialName = t.readToEndOfLine();
									groups.add(positionIndices.size());
									usedMaterials.add(materialName);
								}
								else
									if (t.match("mtllib ")) {
										string materialLibName = t.readToEndOfLine();
										importMtlFile(subDir, materialLibName);
									}
									else
										t.skipCurrentLine();
		}
		if (groups.size() == 0) // no material used
		{
			usedMaterials.add("default");
			groups.add(0);
		}
		groups.add(positionIndices.size());

		fclose(stream);

		float3* computedNormals = new float3[positions.size()];

		float3* lineVertices = new float3[lpositionIndices.size()];
		for (int i = 0; i < lpositionIndices.size(); i++)
			lineVertices[i] = positions[lpositionIndices[i] - 1];
		//LineVB = manager->builder->StructuredBuffer(lineVertices, lpositionIndices.getCount());

		VerticesDataLength = positionIndices.size();
		VerticesData = new SCENE_VERTEX[VerticesDataLength];
		ObjectsId = new int[VerticesDataLength];

		float scaleX = Maxim.x - Minim.x;
		float scaleY = Maxim.y - Minim.y;
		float scaleZ = Maxim.z - Minim.z;
		float scale = max(0.01f, min(scaleX, min(scaleY, scaleZ)));

		for (int i = 0; i < positionIndices.size(); i++)
		{
			VerticesData[i].Position = positions[positionIndices[i] - 1] / scale;
		}

		for (int i = 0; i < positionIndices.size(); i++)
			if (normalIndices[i] == 0)
			{
				float3 p0 = VerticesData[(i / 3) * 3 + 0].Position;
				float3 p1 = VerticesData[(i / 3) * 3 + 1].Position;
				float3 p2 = VerticesData[(i / 3) * 3 + 2].Position;
				computedNormals[positionIndices[i] - 1] = computedNormals[positionIndices[i] - 1] + (cross(p1 - p0, p2 - p0));
			}

		for (int i = 0; i < positionIndices.size(); i++)
			if (normalIndices[i] != 0)
				VerticesData[i].Normal = normals[normalIndices[i] - 1];
			else
			{
				VerticesData[i].Normal = normalize(computedNormals[positionIndices[i] - 1]);
				/*float3 p0 = VerticesData[(i / 3) * 3 + 0].Position;
				float3 p1 = VerticesData[(i / 3) * 3 + 1].Position;
				float3 p2 = VerticesData[(i / 3) * 3 + 2].Position;
				VerticesData[i].Normal = normalize(cross(p1 - p0, p2 - p0));*/
			}
		
		for (int i = 0; i < positionIndices.size(); i++)
			if (textureIndices[i] != 0)
				VerticesData[i].Coordinates = texcoords[textureIndices[i] - 1];
			else
				VerticesData[i].Coordinates = positions[positionIndices[i] - 1].getXY() / float2(scaleX, scaleY) - float2(0.5f, 0.5f);

#pragma region Compute Tangents
		ComputeTangents();
#pragma endregion

		ObjectsLength = groups.size() - 1;

		MaterialIndices = new int[ObjectsLength];
		TransformsData = new float4x4[ObjectsLength];
		Objects = new SCENE_OBJECT[ObjectsLength];

		for (int i = 0; i < ObjectsLength; i++)
		{
			int startIndex = groups[i];
			int count = groups[i + 1] - groups[i];

			for (int j = 0; j < count; j++)
				ObjectsId[startIndex + j] = i;

			MaterialIndices[i] = getMaterialIndex(usedMaterials[i]);
			TransformsData[i] = float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
			Objects[i] = {
				startIndex,
				count,
				&MaterialsData[MaterialIndices[i]],
				&TransformsData[i]
			};
		}

#ifdef SHUFFLE
#pragma region Shuffle
		for (int i = 1; i < positionIndices.getCount() / 3; i++)
		{
			int changeWith = rand() % i;
			for (int j = 0; j < 3; j++)
			{
				VERTEX tmp = vertices[i * 3 + j];
				vertices[i * 3 + j] = vertices[changeWith * 3 + j];
				vertices[changeWith * 3 + j] = tmp;

				int tmp2 = objectId[i * 3 + j];
				objectId[i * 3 + j] = objectId[changeWith * 3 + j];
				objectId[changeWith * 3 + j] = tmp2;
			}
		}
#pragma endregion
#endif
	}
};