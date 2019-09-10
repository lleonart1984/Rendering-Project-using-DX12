#include "../../stdafx.h"
#define RES 512

class SceneGeometryConstruction : public Technique, public IHasScene {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;

    class SceneGeometryPipeline : public GraphicsPipelineBindings {
    public:
        gObj<Texture2D> renderTarget;
        gObj<Buffer> cameraCB;

        // UAV
        gObj<Buffer> counting;

        // SRV
        gObj<Buffer> vertices;
        gObj<Buffer> objectIds;
        gObj<Buffer> transforms;
        gObj<Buffer> materialIndices;
        gObj<Buffer> materials;

        gObj<Texture2D>* Textures = nullptr;
        int TextureCount;

    protected:
        void OnSetup() {
            _ gSet InputLayout(SCENE_VERTEX::Layout());
            _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\WorldViewTransformVS.cso"));
            _ gSet GeometryShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CrumblingGS.cso"));
            _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\TriangleStorePS.cso"));
        }

        void Globals() {
            RTV(0, renderTarget);

            CBV(0, cameraCB, ShaderType_Vertex);
            SRV(0, vertices, ShaderType_Vertex);
            SRV(1, objectIds, ShaderType_Vertex);
            SRV(2, transforms, ShaderType_Vertex);

            Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);
            UAV(0, counting, ShaderType_Pixel);
            SRV(0, materialIndices, ShaderType_Pixel);
            SRV(1, materials, ShaderType_Pixel);
            SRV_Array(2, Textures, TextureCount, ShaderType_Pixel);
        }
    };

    gObj<SceneGeometryPipeline> pipeline;
    const int width, height;

    SceneGeometryConstruction(int width, int height) : width(width), height(height) {
    }
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;

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

        pipeline->renderTarget = _ gCreate DrawableTexture2D<float4>(width, height);

        // Create globals VS constant buffer
        pipeline->cameraCB = _ gCreate ConstantBuffer<Globals>();
        pipeline->counting = _ gCreate IndexBuffer<int>(1);

        pipeline->TextureCount = sceneLoader->TextureCount;
        pipeline->Textures = sceneLoader->Textures;
        pipeline->vertices = sceneLoader->VertexBuffer;
        pipeline->objectIds = sceneLoader->ObjectBuffer;
        pipeline->materialIndices = sceneLoader->MaterialIndexBuffer;
        pipeline->materials = sceneLoader->MaterialBuffer;
        pipeline->transforms = sceneLoader->TransformBuffer;
    }

    void Graphics(gObj<GraphicsManager> manager) {
        manager gCopy ValueData(pipeline->cameraCB, Globals{ ProjectionMatrix, ViewMatrix });

        manager gClear RT(pipeline->renderTarget, float4(0, 0, 0, 0));

        manager gSet Viewport(width, height);
        manager gSet Pipeline(pipeline);

        manager gDispatch Triangles(sceneLoader->VertexBuffer->ElementCount);
    }

    void Frame() {
        perform(Graphics);
    }
};