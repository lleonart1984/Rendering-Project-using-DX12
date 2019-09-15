#pragma once

#include "APITConstruction.h"

class DebugAPIT_Pipeline : public ShowTexturePipeline {
public:
    gObj<Texture2D> renderTarget;

    // CBs
    gObj<Buffer> screenInfo;

    // SRVs
    gObj<Texture2D> rootBuffer;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> boundaryBuffer;
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> preorderBuffer;
    gObj<Buffer> skipBuffer;
    gObj<Buffer> depth;
protected:
    void Setup() {
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\DebugAPIT_PS.cso"));
    }

    void Globals() {
        RTV(0, renderTarget);

        CBV(0, screenInfo, ShaderType_Pixel);
        SRV(0, rootBuffer, ShaderType_Pixel);
        SRV(1, firstBuffer, ShaderType_Pixel);
        SRV(2, boundaryBuffer, ShaderType_Pixel);
        SRV(3, nodeBuffer, ShaderType_Pixel);
        SRV(4, nextBuffer, ShaderType_Pixel);
        SRV(5, preorderBuffer, ShaderType_Pixel);
        SRV(6, skipBuffer, ShaderType_Pixel);
        SRV(7, depth, ShaderType_Pixel);
    }
};

class DebugAPIT : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<APITConstruction> aPIT;

    gObj<DebugAPIT_Pipeline> pipeline;

    gObj<Buffer> vertices;

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

        aPIT = new APITConstruction(7);
        aPIT->sceneLoader = sceneLoader;
        _ gLoad Subprocess(aPIT);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);

        // Create globals VS constant buffer
        pipeline->screenInfo = aPIT->aBuffer->screenInfo;
        pipeline->rootBuffer = aPIT->RootBuffer;
        pipeline->firstBuffer = aPIT->FirstBuffer;
        pipeline->boundaryBuffer = aPIT->BoundaryBuffer;
        pipeline->nodeBuffer = aPIT->NodeBuffer;
        pipeline->nextBuffer = aPIT->NextBuffer;
        pipeline->preorderBuffer = aPIT->PreorderBuffer;
        pipeline->skipBuffer = aPIT->SkipBuffer;
        pipeline->depth = aPIT->Depth;

        perform(CreatingAssets);
    }

    // A copy engine can be used to populate buffers using GPU commands.
    void CreatingAssets(gObj<CopyingManager> manager) {
        vertices = _ gCreate VertexBuffer<float2>(6);

        // Copies a buffer written using an initializer_list
        manager gCopy ListData(vertices, {
                float2(-1, 1),
                float2(1, 1),
                float2(1, -1),

                float2(1, -1),
                float2(-1, -1),
                float2(-1, 1),
            });
    }

    void Graphics(gObj<GraphicsManager> manager) {
        pipeline->renderTarget = render_target;

        float4x4 view, proj;
        Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
        aPIT->ViewMatrix = view;
        ExecuteFrame(aPIT);

        manager gClear RT(render_target, float4(Backcolor, 1));

        manager gSet Viewport(render_target->Height * 3.0 / 4.0, render_target->Height);
        manager gSet Pipeline(pipeline);
        manager gSet VertexBuffer(vertices);

        manager gDispatch Triangles(6);
    }

    void Frame() {
        perform(Graphics);
    }
};