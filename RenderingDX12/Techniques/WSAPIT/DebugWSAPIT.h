#pragma once

#include "WorldSpaceAPIT.h"

class DebugWSAPIT_Pipeline : public GraphicsPipelineBindings {
public:
    gObj<Texture2D> renderTarget;

    // CBVs
    gObj<Buffer> cameraCB;

    // SRVs
    gObj<Buffer> wsVertices;
    gObj<Buffer> sceneBoundaries;
protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\DebugWSAPIT_VS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\DebugWSAPIT_PS.cso"));
    }

    void Globals() {
        RTV(0, renderTarget);

        CBV(0, cameraCB, ShaderType_Vertex);
        SRV(0, wsVertices, ShaderType_Vertex);
        
        SRV(0, sceneBoundaries, ShaderType_Pixel);
    }
};

class DebugWSAPIT : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<WorldSpaceTransformer> worldSpaceTransformer;

    gObj<DebugWSAPIT_Pipeline> pipeline;

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

        worldSpaceTransformer = new WorldSpaceTransformer();
        worldSpaceTransformer->sceneLoader = sceneLoader;
        _ gLoad Subprocess(worldSpaceTransformer);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);
        pipeline->cameraCB = _ gCreate ConstantBuffer<Globals>();
        pipeline->wsVertices = worldSpaceTransformer->WorldSpaceVertices;
        pipeline->sceneBoundaries = worldSpaceTransformer->SceneBoundaries;

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
        ExecuteFrame(worldSpaceTransformer);

        manager gCopy ValueData(pipeline->cameraCB, Globals{ proj, view });

        manager gClear RT(render_target, float4(Backcolor, 1));
        manager gSet Viewport(render_target->Width, render_target->Height);
        manager gSet Pipeline(pipeline);
        manager gDispatch Triangles(sceneLoader->VertexBuffer->ElementCount);
        /*manager gSet Viewport(render_target->Height * 3.0 / 4.0, render_target->Height);
        manager gSet Pipeline(pipeline);
        manager gSet VertexBuffer(vertices);

        manager gDispatch Triangles(6);*/
    }

    void Frame() {
        perform(Graphics);
    }
};