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

class WorldSpaceTransformer : public Technique, public IHasScene {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<WorldSpaceTransformerPipeline> pipeline;
    gObj<Buffer> WorldSpaceVertices;
    gObj<Buffer> SceneBoundaries;

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
        _ gLoad Pipeline(pipeline);

        WorldSpaceVertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
        SceneBoundaries = _ gCreate RWStructuredBuffer<int3>(2);
        
        pipeline->worldSpaceVertices = WorldSpaceVertices;
        pipeline->sceneBoundaries = SceneBoundaries;

        pipeline->vertices = sceneLoader->VertexBuffer;
        pipeline->objectIds = sceneLoader->ObjectBuffer;
        pipeline->transforms = sceneLoader->TransformBuffer;
    }

    void Compute(gObj<GraphicsManager> manager) {
        manager gSet Pipeline(pipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_LINEARGROUP(pipeline->vertices->ElementCount));
    }

    void Frame() {
        perform(Compute);
    }
};