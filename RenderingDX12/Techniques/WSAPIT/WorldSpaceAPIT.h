#pragma once
#include "../../stdafx.h"
#include "../Common/CS_Constants.h"

class WorldSpaceTransformerPipeline : public ComputePipelineBindings {
public:
    // SRV
    gObj<Buffer> vertices;
    gObj<Buffer> objectIds;
    gObj<Buffer> transforms;

    // UAV
    gObj<Buffer> worldSpaceVertices;
    gObj<Buffer> sceneBoundaries;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WorldSpaceTransform_CS.cso"));
    }

    void Globals() {
        SRV(0, vertices, ShaderType_Any);
        SRV(1, objectIds, ShaderType_Any);
        SRV(2, transforms, ShaderType_Any);

        UAV(0, worldSpaceVertices, ShaderType_Any);
        UAV(1, sceneBoundaries, ShaderType_Any);
    }
};

class WorldSpaceFillPipeline : public GraphicsPipelineBindings {
public:
    gObj<Buffer> screenInfo;

    gObj<Buffer> wsVertices;
    gObj<Buffer> sceneBoundaries;

    gObj<Buffer> fragments;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;

protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Fill_VS.cso"));
        _ gSet GeometryShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Fill_GS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Fill_PS.cso"));
    }

    void Globals() {
        SRV(0, wsVertices, ShaderType_Vertex);
        SRV(1, sceneBoundaries, ShaderType_Vertex);

        CBV(0, screenInfo, ShaderType_Geometry);

        CBV(0, screenInfo, ShaderType_Pixel);
        UAV(0, fragments, ShaderType_Pixel);
        UAV(1, firstBuffer, ShaderType_Pixel);
        UAV(2, nextBuffer, ShaderType_Pixel);
        UAV(3, malloc, ShaderType_Pixel);
    }
};

class WorldSpaceAPIT : public Technique, public IHasScene {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<Buffer> WorldSpaceVertices;
    gObj<Buffer> SceneBoundaries;
    gObj<Buffer> Fragments;
    gObj<Buffer> RootBuffer;
    gObj<Buffer> NextBuffer;
    gObj<Buffer> Malloc;
    gObj<Buffer> screenInfo;

    gObj<WorldSpaceTransformerPipeline> wsTransformPipeline;
    gObj<WorldSpaceFillPipeline> fillPipeline;

    APITDescription description;

    WorldSpaceAPIT(APITDescription description) {
        this->description = description;
    }

protected:
    void SetScene(gObj<CA4G::Scene> scene) {
        IHasScene::SetScene(scene);
        if (sceneLoader != nullptr) {
            sceneLoader->SetScene(scene);
        }
    }

    void Startup() {
        // Load scene in retained mode
        if (sceneLoader == nullptr) {
            sceneLoader = new RetainedSceneLoader();
            sceneLoader->SetScene(Scene);
            _ gLoad Subprocess(sceneLoader);
        }

        // Load and setup pipeline resource
        _ gLoad Pipeline(wsTransformPipeline);
        _ gLoad Pipeline(fillPipeline);

        int resolution = description.getResolution();
        WorldSpaceVertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
        SceneBoundaries = _ gCreate RWStructuredBuffer<int3>(2);
        Fragments = _ gCreate RWStructuredBuffer<Fragment>(MAX_NUMBER_OF_FRAGMENTS);
        RootBuffer = _ gCreate DrawableTexture2D<int>(resolution, resolution);
        NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        Malloc = _ gCreate RWStructuredBuffer<int>(1);
        screenInfo = _ gCreate ConstantBuffer<ScreenInfo>();
        
        wsTransformPipeline->worldSpaceVertices = WorldSpaceVertices;
        wsTransformPipeline->sceneBoundaries = SceneBoundaries;
        wsTransformPipeline->vertices = sceneLoader->VertexBuffer;
        wsTransformPipeline->objectIds = sceneLoader->ObjectBuffer;
        wsTransformPipeline->transforms = sceneLoader->TransformBuffer;

        fillPipeline->wsVertices = WorldSpaceVertices;
        fillPipeline->sceneBoundaries = SceneBoundaries;
        fillPipeline->fragments = Fragments;
        fillPipeline->firstBuffer = RootBuffer;
        fillPipeline->nextBuffer = NextBuffer;
        fillPipeline->malloc = Malloc;
        fillPipeline->screenInfo = screenInfo;
    }

    void Compute(gObj<GraphicsManager> manager) {
        manager gSet Pipeline(wsTransformPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_LINEARGROUP(wsTransformPipeline->vertices->ElementCount));

        int resolution = description.getResolution();
        manager gClear UAV(Malloc, 0U);
        manager gClear UAV(RootBuffer, (unsigned int)-1);
        manager gCopy ValueData(screenInfo, ScreenInfo{ resolution, resolution });

        manager gSet Viewport(resolution, resolution);
        manager gSet Pipeline(fillPipeline);
        
        manager gDispatch Triangles(fillPipeline->wsVertices->ElementCount);
    }

    void Frame() {
        perform(Compute);
    }
};