#pragma once

#include "ca4g.h"

#include "ModelLoaders/ObjLoader.h"

using namespace std;

#pragma region Common scene definition objects

// Represents a camera given a position and look target.
class Camera
{
public:
	// Gets or sets the position of the observer
	float3 Position;
	// Gets or sets the target position of the observer
	// Can be used normalize(Target-Position) to refer to look direction
	float3 Target;
	// Gets or sets a vector used as constraint of camera normal.
	float3 Up;
	// Gets or sets the camera field of view in radians (vertical angle).
	float FoV;
	// Gets or sets the distance to the nearest visible plane for this camera.
	float NearPlane;
	// Gets or sets the distance to the farthest visible plane for this camera.
	float FarPlane;

	// Rotates this camera around Position
	void Rotate(float hor, float vert) {
		float3 dir = normalize(Target - Position);
		float3x3 rot = ::Rotate(hor, Up);
		dir = mul(dir, rot);
		float3 R = cross(dir, Up);
		if (length(R) > 0)
		{
			rot = ::Rotate(vert, R);
			dir = mul(dir, rot);
			Target = Position + dir;
		}
	}

	// Rotates this camera around Target
	void RotateAround(float hor, float vert) {
		float3 dir = (Position - Target);
		float3x3 rot = ::Rotate(hor, float3(0, 1, 0));
		dir = mul(dir, rot);
		Position = Target + dir;
	}

	// Move this camera in front direction
	void MoveForward(float speed = 0.01) {
		float3 dir = normalize(Target - Position);
		Target = Target + dir * speed;
		Position = Position + dir * speed;
	}

	// Move this camera in backward direction
	void MoveBackward(float speed = 0.01) {
		float3 dir = normalize(Target - Position);
		Target = Target - dir * speed;
		Position = Position - dir * speed;
	}

	// Move this camera in right direction
	void MoveRight(float speed = 0.01) {
		float3 dir = Target - Position;
		float3 R = normalize(cross(dir, Up));

		Target = Target + R * speed;
		Position = Position + R * speed;
	}

	// Move this camera in left direction
	void MoveLeft(float speed = 0.01) {
		float3 dir = Target - Position;
		float3 R = normalize(cross(dir, Up));

		Target = Target - R * speed;
		Position = Position - R * speed;
	}

	// Gets the matrices for view and projection transforms
	void GetMatrices(int screenWidth, int screenHeight, float4x4 &view, float4x4 &projection)
	{
		view = LookAtRH(Position, Target, Up);
		projection = PerspectiveFovRH(FoV, screenHeight / (float)screenWidth, NearPlane, FarPlane);
	}
};

// Represents a light source definition
// Point sources (Position: position of the light source, Direction = 0, SpotAngle = 0)
// Directional sources (Position = 0, Direction: direction of the light source, SpotAngle = 0)
// Spot lights (Position: position of the light source, Direction: direction of the spot light, SpotAngle: fov)
class LightSource
{
public:
	float3 Position;
	float3 Direction;
	float SpotAngle;
	// Gets the intensity of the light
	float3 Intensity;
};


#pragma endregion

class Scene {

public:

	inline SCENE_VERTEX* Vertices() {
		return loader->VerticesData;
	}
	inline int VerticesCount() {
		return loader->VerticesDataLength;
	}
	inline int* ObjectIds() {
		return loader->ObjectsId;
	}
	inline list<SCENE_MATERIAL>& Materials() {
		return loader->MaterialsData;
	}
	inline int MaterialsCount() {
		return loader->MaterialsData.size();
	}
	inline int ObjectsCount() {
		return loader->ObjectsLength;
	}
	inline int* MaterialIndices() {
		return loader->MaterialIndices;
	}
	inline SCENE_OBJECT* Objects() {
		return loader->Objects;
	}
	inline float4x4* Transforms() {
		return loader->TransformsData;
	}
	inline list<TextureData*>& Textures() {
		return loader->Textures;
	}

	Scene(const char* objFilePath) {
		
		int len = strlen(objFilePath);

		// It is obj model
		if (strcmp(objFilePath + len - 4, ".obj") == 0)
			loader = new ObjLoader();

		if (loader != nullptr)
			loader->Load(objFilePath);
	}
private:
	BaseLoader* loader = nullptr;
};

#pragma region Technique sub-classing

struct IHasBackcolor {
	float3 Backcolor;
private:
	virtual void Boo() {}
};

struct IHasCamera {
	Camera* Camera;
private:
	virtual void Boo() {}
};

struct IHasLight {
	LightSource* Light;
private:
	virtual void Boo() {}
};

struct IHasScene {
	Scene* Scene;
private:
	virtual void Boo() {}
};

#define MAX_NUMBER_OF_TRIANGLES 10000

struct IHasTriangleNumberParameter {
	int NumberOfTriangles = 1000;
	int MinimumOfTriangles = 0;
	int MaximumOfTriangles = MAX_NUMBER_OF_TRIANGLES;
};

struct IHasParalellism {
	int NumberOfWorkers = 1;
};

#pragma endregion