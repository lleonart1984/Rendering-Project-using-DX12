#pragma once
#include "stdafx.h"

#define BUNNY_OBJ 0
#define RING_OBJ 1
#define SPONZA_OBJ 2
#define SIBENIK_OBJ 3
#define SANMIGUEL_OBJ 4

#define USE_SCENE RING_OBJ  

// Change this to force every frame camera dirty
#define PERMANENT_CAMERA_DIRTY false

// Uncomment this to use warp device for unsupported DX12 functionalities in your device
//#define WARP

// Uncomment this to Force fallback device
//#define FORCE_FALLBACK

//class NoTechnique : public Technique {}; gObj<NoTechnique> technique;

//gObj<Tutorial1> technique;
//gObj<Tutorial2> technique;
//gObj<Tutorial3> technique;
//gObj<Tutorial4> technique;
//gObj<Tutorial5> technique;
//gObj<Tutorial6> technique;
//gObj<Tutorial7> technique;
//gObj<Tutorial8> technique;
//gObj<Tutorial9> technique;
//gObj<Tutorial10> technique;
//gObj<Tutorial11> technique;
//gObj<Tutorial12> technique;
//gObj<SampleTechnique> technique;
//gObj<RetainedBasicRenderer> technique;
//gObj<DXRBasicSceneRaytracer> technique;
//gObj<DXRBasicScenePhotontracer> technique;
//gObj<DXRItPathtracer> technique;
//gObj<DeferredShadingTechnique> technique;
gObj<HybridPhotonTracer> technique;
//gObj<DXRPathtracer> technique;
//gObj<FullDXRPhotonTracer2> technique; 
//gObj<FullDXRPhotonTracer> technique;