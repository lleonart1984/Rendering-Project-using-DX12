#pragma once

#include "WorldSpaceMRAPIT.h"

class DebugWSMRAPIT_Pipeline : public GraphicsPipelineBindings {
public:
    gObj<Texture2D> renderTarget;

    // CBVs
    gObj<Buffer> cameraCB;
    gObj<Buffer> screenInfo;

    // SRVs
    gObj<Buffer> wsVertices;
    gObj<Buffer> sceneBoundaries;
    gObj<Buffer> fragments;
    gObj<Buffer> rootBuffer;
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> preorderBuffer;
    gObj<Buffer> skipBuffer;

    gObj<Buffer> boundaries;
protected:
    void Setup() {
        //ShowTexturePipeline::Setup();
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\WS_MRAPIT\\Shaders\\DebugWSMRAPIT_VS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\WS_MRAPIT\\Shaders\\DebugWSMRAPIT_PS.cso"));
    }

    void Globals() {
        RTV(0, renderTarget);

        CBV(0, cameraCB, ShaderType_Vertex);
        SRV(0, wsVertices, ShaderType_Vertex);

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

        SRV(9, boundaries, ShaderType_Pixel);
    }
};

class DebugWSMRAPIT : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<WorldSpaceMRAPIT> worldSpaceMRAPIT;
    gObj<DebugWSMRAPIT_Pipeline> pipeline;
    MRAPITDescription description;

    gObj<Buffer> debugVertices;

    DebugWSMRAPIT(MRAPITDescription description) {
        this->description = description;
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

        worldSpaceMRAPIT = new WorldSpaceMRAPIT(description);
        worldSpaceMRAPIT->sceneLoader = sceneLoader;
        _ gLoad Subprocess(worldSpaceMRAPIT);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);
        pipeline->screenInfo = worldSpaceMRAPIT->screenInfo;
        pipeline->sceneBoundaries = worldSpaceMRAPIT->SceneBoundaries;
        pipeline->fragments = worldSpaceMRAPIT->Fragments;
        pipeline->rootBuffer = worldSpaceMRAPIT->RootBuffer;
        pipeline->nodeBuffer = worldSpaceMRAPIT->NodeBuffer;
        pipeline->boundaryNodeBuffer = worldSpaceMRAPIT->BoundaryNodeBuffer;
        pipeline->firstBuffer = worldSpaceMRAPIT->FirstBuffer;
        pipeline->nextBuffer = worldSpaceMRAPIT->NextBuffer;
        pipeline->preorderBuffer = worldSpaceMRAPIT->PreorderBuffer;
        pipeline->skipBuffer = worldSpaceMRAPIT->SkipBuffer;
        pipeline->boundaries = worldSpaceMRAPIT->boundaries;
        
        pipeline->cameraCB = _ gCreate ConstantBuffer<Globals>();
        pipeline->wsVertices = worldSpaceMRAPIT->Vertices;

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
        ExecuteFrame(worldSpaceMRAPIT);

        float4x4 view, proj;
        Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
        manager gCopy ValueData(pipeline->cameraCB, Globals{ proj, view });
        
        pipeline->renderTarget = render_target;
        manager gClear RT(render_target, float4(Backcolor, 1));
        manager gSet Viewport(render_target->Width, render_target->Height);
        manager gSet Pipeline(pipeline);
        /*manager gSet VertexBuffer(debugVertices);

        manager gDispatch Triangles(6);*/

        manager gDispatch Triangles(sceneLoader->VertexBuffer->ElementCount);
    }

    void Frame() {
        perform(Graphics);
    }
};