#pragma once


#pragma region Technique sub-classing

// Defines this technique will use a back color info
struct IHasBackcolor {
	float3 Backcolor;
private:
	virtual void Boo() {}
};

struct IHasAccumulative {
	int CurrentFrame;
	int StopFrame;
private:
	virtual void Boo() {}
};

// Defines this technique will use a camera
struct IHasCamera {
	Camera* Camera;
	bool CameraIsDirty;
private:
	virtual void Boo() {}
};

// Defines this technique will use a light settings
struct IHasLight {
	LightSource* Light;
	bool LightSourceIsDirty;
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

struct IHasVolume {
	gObj<Volume> Volume;

	float densityScale = 300;
	float globalAbsortion = 0.0;

	virtual void SetVolume(gObj<CA4G::Volume> volume) {
		this->Volume = volume;
	}
};

struct IHasHomogeneousVolume {
	float densityScale = 1;
	float globalAbsortion = 0.0;
	float gFactor = 0.875;

	virtual void Boo() {}
};

struct IHasScatteringEvents {
	float density = 10;
	float3 scatteringAlbedo = float3(1, 1, 1);
	float3 gFactor = float3(1, 1, 1) * 0.0f;// 0.875;
	float pathtracing = 0.5;
	float3 phi = float3(1.0, 1.0, 1.0);
	bool CountSteps = false;

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

struct IHasRaymarchDebugInfo {
	bool CountHits;
	bool CountSteps;
private:
	virtual void Boo() {}
};

#pragma endregion