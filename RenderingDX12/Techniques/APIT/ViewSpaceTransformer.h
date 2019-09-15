#include "../../stdafx.h"

class ViewSpaceTransformer : public Technique, public IHasScene {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;

    class ViewSpaceTransformerPipeline :
#ifdef USE_COMPUTESHADER
        public ComputePipelineBindings {
        ShaderType VS_Type = ShaderType_Any;
        ShaderType PS_Type = ShaderType_Any;
#else
        public GraphicsPipelineBindings {
        ShaderType VS_Type = ShaderType_Vertex;
        ShaderType PS_Type = ShaderType_Pixel;
#endif
    public:
        gObj<Buffer> viewSpaceVertices;
        gObj<Buffer> cameraCB;

        // SRV
        gObj<Buffer> vertices;
        gObj<Buffer> objectIds;
        gObj<Buffer> transforms;

    protected:
        void Setup() {
#ifdef USE_COMPUTESHADER
            _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\ViewSpaceTransform_CS.cso"));
#else
            _ gSet InputLayout({});
            _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\ViewSpaceTransform_VS.cso"));
            _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\ViewSpaceTransform_PS.cso"));
#endif
        }

        void Globals() {
            CBV(0, cameraCB, VS_Type);
            SRV(0, vertices, VS_Type);
            SRV(1, objectIds, VS_Type);
            SRV(2, transforms, VS_Type);

            UAV(0, viewSpaceVertices, PS_Type);
        }
    };
    gObj<ViewSpaceTransformerPipeline> pipeline;

    gObj<Buffer> ViewSpaceVertices;
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

        ViewSpaceVertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
        // Create globals VS constant buffer
        pipeline->cameraCB = _ gCreate ConstantBuffer<float4x4>();
        pipeline->viewSpaceVertices = ViewSpaceVertices;

        pipeline->vertices = sceneLoader->VertexBuffer;
        pipeline->objectIds = sceneLoader->ObjectBuffer;
        pipeline->transforms = sceneLoader->TransformBuffer;
    }

    void Compute(gObj<GraphicsManager> manager) {
        manager gCopy ValueData(pipeline->cameraCB, ViewMatrix);

#ifdef USE_COMPUTESHADER
        manager gSet Pipeline(pipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(pipeline->vertices->ElementCount);
#else
        manager gSet Viewport(1, 1);
        manager gSet Pipeline(pipeline);
        manager gDispatch Primitive(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, pipeline->vertices->ElementCount);
#endif
    }

    void Frame() {
        perform(Compute);
    }
};