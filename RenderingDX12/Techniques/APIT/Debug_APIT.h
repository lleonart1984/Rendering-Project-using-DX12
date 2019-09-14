#pragma once

#include "ABuffer.h"

class Debug_APITPipeline : public ShowTexturePipeline {
public:
    gObj<Texture2D> renderTarget;

    // CBs
    gObj<Buffer> screenInfo;

    // SRVs
    gObj<Texture2D> firstBuffer;
    gObj<Buffer> nextBuffer;
protected:
    void Setup() {
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\TestPS.cso"));
    }

    void Globals() {
        RTV(0, renderTarget);

        CBV(0, screenInfo, ShaderType_Pixel);
        SRV(0, firstBuffer, ShaderType_Pixel);
        SRV(1, nextBuffer, ShaderType_Pixel);
    }
};

class Debug_APIT : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<ABuffer> abuffer;

    gObj<Debug_APITPipeline> pipeline;

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

        abuffer = new ABuffer(render_target->Width, render_target->Height);
        abuffer->sceneLoader = sceneLoader;
        _ gLoad Subprocess(abuffer);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);

        // Create globals VS constant buffer

        pipeline->screenInfo = abuffer->screenInfo;

        pipeline->firstBuffer = abuffer->firstBuffer;
        pipeline->nextBuffer = abuffer->nextBuffer;

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
        abuffer->ViewMatrix = view;
        ExecuteFrame(abuffer);

        manager gClear RT(render_target, float4(Backcolor, 1));

        manager gSet Viewport(render_target->Height, render_target->Height);
        manager gSet Pipeline(pipeline);
        manager gSet VertexBuffer(vertices);

        manager gDispatch Triangles(6);
    }

    void Frame() {
        perform(Graphics);
    }
};