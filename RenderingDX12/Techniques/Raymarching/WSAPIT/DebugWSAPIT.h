#pragma once

#include "WorldSpaceAPIT.h"

class DebugWSAPIT_Pipeline : public ShowTexturePipeline {
public:
    gObj<Texture2D> renderTarget;

    // CBVs
    gObj<Buffer> screenInfo;

    // SRVs
    gObj<Buffer> sceneBoundaries;
    gObj<Buffer> fragments;
    gObj<Texture2D> rootBuffer;
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> preorderBuffer;
    gObj<Buffer> skipBuffer;
protected:
    void Setup() {
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\DebugWSAPIT_PS.cso"));
    }

    void Globals() {
        RTV(0, renderTarget);

        CBV(0, screenInfo, ShaderType_Pixel);
        SRV(0, sceneBoundaries, ShaderType_Pixel);
        SRV(1, fragments, ShaderType_Pixel);
        SRV(2, rootBuffer, ShaderType_Pixel);
        SRV(3, nodeBuffer, ShaderType_Pixel);
        SRV(4, boundaryNodeBuffer, ShaderType_Pixel);
        SRV(5, firstBuffer, ShaderType_Pixel);
        SRV(6, nextBuffer, ShaderType_Pixel);
        SRV(7, preorderBuffer, ShaderType_Pixel);
        SRV(8, skipBuffer, ShaderType_Pixel);
    }
};

class DebugWSAPIT : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<WorldSpaceAPIT> worldSpaceAPIT;
    gObj<DebugWSAPIT_Pipeline> pipeline;
    APITDescription aPITDescription;

    gObj<Buffer> debugVertices;

    DebugWSAPIT(APITDescription description) {
        this->aPITDescription = description;
    }

protected:
    void SetScene(gObj<CA4G::Scene> scene) {
        IHasScene::SetScene(scene);
        if (sceneLoader != nullptr)
            sceneLoader->SetScene(scene);
    }

    void Startup() {
        if (sceneLoader == nullptr) {
            sceneLoader = new RetainedSceneLoader();
            sceneLoader->SetScene(this->Scene);
            _ gLoad Subprocess(sceneLoader);
        }

        worldSpaceAPIT = new WorldSpaceAPIT(aPITDescription);
        worldSpaceAPIT->sceneLoader = sceneLoader;
        _ gLoad Subprocess(worldSpaceAPIT);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);
        pipeline->screenInfo = worldSpaceAPIT->screenInfo;
        pipeline->sceneBoundaries = worldSpaceAPIT->SceneBoundaries;
        pipeline->fragments = worldSpaceAPIT->Fragments;
        pipeline->rootBuffer = worldSpaceAPIT->RootBuffer;
        pipeline->nodeBuffer = worldSpaceAPIT->NodeBuffer;
        pipeline->boundaryNodeBuffer = worldSpaceAPIT->BoundaryNodeBuffer;
        pipeline->firstBuffer = worldSpaceAPIT->FirstBuffer;
        pipeline->nextBuffer = worldSpaceAPIT->NextBuffer;
        pipeline->preorderBuffer = worldSpaceAPIT->PreorderBuffer;
        pipeline->skipBuffer = worldSpaceAPIT->SkipBuffer;

        perform(CreatingAssets);
    }

    // A copy engine can be used to populate buffers using GPU commands.
    void CreatingAssets(gObj<CopyingManager> manager) {
        debugVertices = _ gCreate VertexBuffer<float2>(6);

        // Copies a buffer written using an initializer_list
        manager gCopy ListData(debugVertices, {
                float2(-1, 1),
                float2(1, 1),
                float2(1, -1),

                float2(1, -1),
                float2(-1, -1),
                float2(-1, 1),
            });
    }

    void Graphics(gObj<GraphicsManager> manager) {
        ExecuteFrame(worldSpaceAPIT);
        
        pipeline->renderTarget = render_target;
        manager gClear RT(render_target, float4(Backcolor, 1));
        manager gSet Viewport(render_target->Width, render_target->Height);
        manager gSet Pipeline(pipeline);
        manager gSet VertexBuffer(debugVertices);

        manager gDispatch Triangles(6);
    }

    void Frame() {
        perform(Graphics);
    }
};