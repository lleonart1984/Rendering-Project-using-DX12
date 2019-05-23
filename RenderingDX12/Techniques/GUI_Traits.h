#pragma once


#pragma region Technique sub-classing

// Defines this technique will use a back color info
struct IHasBackcolor {
	float3 Backcolor;
private:
	virtual void Boo() {}
};

// Defines this technique will use a camera
struct IHasCamera {
	Camera* Camera;
private:
	virtual void Boo() {}
};

// Defines this technique will use a light settings
struct IHasLight {
	LightSource* Light;
private:
	virtual void Boo() {}
};

// Defines this technique will use a scene object to draw
struct IHasScene {
	gObj<Scene> Scene;

	virtual void SetScene(gObj<CA4G::Scene> scene) {
		this->Scene = scene;
	}
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