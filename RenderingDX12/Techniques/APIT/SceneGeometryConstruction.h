#include "../../stdafx.h"

class SceneGeometryConstruction : public Technique, public IHasScene {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;

    class SceneGeometryPipeline : public ComputePipelineBindings {
    public:
        gObj<Buffer> transformedVertices;
        gObj<Buffer> cameraCB;

        // UAV
        gObj<Buffer> counting;

        // SRV
        gObj<Buffer> vertices;
        gObj<Buffer> objectIds;
        gObj<Buffer> transforms;

    protected:
        void Setup() {
            _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\WorldViewTransformCS.cso"));
        }

        void Globals() {
            CBV(0, cameraCB, ShaderType_Any);
            SRV(0, vertices, ShaderType_Any);
            SRV(1, objectIds, ShaderType_Any);
            SRV(2, transforms, ShaderType_Any);

            UAV(0, transformedVertices, ShaderType_Any);
        }
    };
    gObj<SceneGeometryPipeline> pipeline;

    gObj<Buffer> TransformedVertices;
    float4x4 ViewMatrix;

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

        TransformedVertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
        // Create globals VS constant buffer
        pipeline->cameraCB = _ gCreate ConstantBuffer<float4x4>();
        pipeline->transformedVertices = TransformedVertices;

        pipeline->vertices = sceneLoader->VertexBuffer;
        pipeline->objectIds = sceneLoader->ObjectBuffer;
        pipeline->transforms = sceneLoader->TransformBuffer;
    }

    void Compute(gObj<GraphicsManager> manager) {
        manager gCopy ValueData(pipeline->cameraCB, ViewMatrix);

        manager gSet Pipeline(pipeline);

        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(pipeline->vertices->ElementCount);
    }

    void Frame() {
        perform(Compute);
    }
};