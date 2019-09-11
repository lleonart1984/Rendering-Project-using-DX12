#pragma once

#include "SceneGeometryConstruction.h"

class APITPipeline : public GraphicsPipelineBindings {
public:
    gObj<Texture2D> renderTarget;
    gObj<Texture2D> depthBuffer;

    // CBs
    gObj<Buffer> cameraCB;
    gObj<Buffer> lighting;

    // SRVs
    gObj<Buffer> vertices;
    gObj<Buffer> objectIds;

    gObj<Buffer> transforms;
    gObj<Buffer> materialIndices;

    gObj<Buffer> materials;

    gObj<Texture2D>* Textures = nullptr;
    int TextureCount;
protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\TestVS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\TestPS.cso"));
        _ gSet DepthTest();
    }

    void Globals() {
        RTV(0, renderTarget);
        DBV(depthBuffer);

        CBV(0, cameraCB, ShaderType_Vertex);
        SRV(0, vertices, ShaderType_Vertex);
        SRV(1, objectIds, ShaderType_Vertex);
        SRV(2, transforms, ShaderType_Vertex);

        Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);
        CBV(0, lighting, ShaderType_Pixel);
        SRV(0, materialIndices, ShaderType_Pixel);
        SRV(1, materials, ShaderType_Pixel);
        SRV_Array(2, Textures, TextureCount, ShaderType_Pixel);
    }
};

class APITRaytracing : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<SceneGeometryConstruction> sceneGeometry;

    gObj<Texture2D> depthBuffer;
    gObj<APITPipeline> pipeline;
    gObj<Buffer> cameraCB;
    gObj<Buffer> lightingCB;

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

        sceneGeometry = new SceneGeometryConstruction();
        sceneGeometry->sceneLoader = sceneLoader;
        _ gLoad Subprocess(sceneGeometry);

        //flush_all_to_gpu;

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);

        // Create depth buffer resource
        gBind(depthBuffer) _ gCreate DepthBuffer(render_target->Width, render_target->Height);

        // Create globals VS constant buffer
        gBind(cameraCB) _ gCreate ConstantBuffer<Globals>();
        gBind(lightingCB) _ gCreate ConstantBuffer<Lighting>();

        pipeline->depthBuffer = depthBuffer;
        pipeline->cameraCB = cameraCB;
        pipeline->lighting = lightingCB;

        pipeline->TextureCount = sceneLoader->TextureCount;
        pipeline->Textures = sceneLoader->Textures;
        pipeline->vertices = sceneGeometry->TransformedVertices;
        pipeline->objectIds = sceneLoader->ObjectBuffer;
        pipeline->materialIndices = sceneLoader->MaterialIndexBuffer;
        pipeline->materials = sceneLoader->MaterialBuffer;
        pipeline->transforms = sceneLoader->TransformBuffer;
    }

    void Graphics(gObj<GraphicsManager> manager) {
        pipeline->renderTarget = render_target;

        float4x4 view, proj;
        Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
        sceneGeometry->ViewMatrix = view;
        ExecuteFrame(sceneGeometry);

        manager gCopy ValueData(cameraCB, Globals{ proj, view });
        manager gCopy ValueData(lightingCB, Lighting{
                    mul(float4(Light->Position, 1), view).getXYZHomogenized(), 0,
                    Light->Intensity
            });

        manager gClear RT(render_target, float4(Backcolor, 1));
        manager gClear Depth(depthBuffer, 1);

        manager gSet Viewport(render_target->Width, render_target->Height);
        manager gSet Pipeline(pipeline);

        manager gDispatch Triangles(sceneLoader->VertexBuffer->ElementCount);
    }

    void Frame() {
        perform(Graphics);
    }
};