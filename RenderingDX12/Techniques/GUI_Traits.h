#pragma once


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