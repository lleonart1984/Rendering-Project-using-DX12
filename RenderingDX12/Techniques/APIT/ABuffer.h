#pragma once

#include "SceneGeometryConstruction.h"

class ABufferPipeline : public GraphicsPipelineBindings {
public:
    gObj<Buffer> screenInfo;

    gObj<Buffer> layerTransforms;
    gObj<Buffer> vertices;

    gObj<Buffer> fragments;
    gObj<Texture2D> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;

protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_VS.cso"));
        _ gSet GeometryShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_GS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_PS.cso"));
        _ gSet NoDepthTest();
    }

    void Globals() {
        // VS
        SRV(0, vertices, ShaderType_Vertex);

        // GS
        CBV(0, screenInfo, ShaderType_Geometry);
        SRV(0, layerTransforms, ShaderType_Geometry);

        // PS
        CBV(0, screenInfo, ShaderType_Pixel);
        UAV(0, fragments, ShaderType_Pixel);
        UAV(1, firstBuffer, ShaderType_Pixel);
        UAV(2, nextBuffer, ShaderType_Pixel);
        UAV(3, malloc, ShaderType_Pixel);
    }
};

class ABuffer : public Technique, public IHasScene {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<SceneGeometryConstruction> sceneGeometry;

    gObj<Buffer> vertices;
    gObj<Buffer> fragments;
    gObj<Texture2D> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;

    gObj<Buffer> screenInfo; // is it necessary?
    gObj<Buffer> layerTransforms;

    gObj<ABufferPipeline> pipeline;
    const int width, height;
    float4x4 ViewMatrix;

    ABuffer(int width, int height)
        : width(width), height(height) { }

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

        sceneGeometry = new SceneGeometryConstruction();
        sceneGeometry->sceneLoader = sceneLoader;
        _ gLoad Subprocess(sceneGeometry);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);

        vertices = sceneGeometry->TransformedVertices;
        fragments = _ gCreate RWStructuredBuffer<Fragment>(MAX_NUMBER_OF_FRAGMENTS);
        firstBuffer = _ gCreate DrawableTexture2D<int>(width * 6, height);
        nextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        malloc = _ gCreate RWStructuredBuffer<int>(1);
        screenInfo = _ gCreate ConstantBuffer<ScreenInfo>();
        layerTransforms = _ gCreate StructuredBuffer<float3x3>(6);

        // Create globals VS constant buffer
        pipeline->screenInfo = screenInfo;

        pipeline->vertices = vertices;
        pipeline->layerTransforms = layerTransforms;

        pipeline->fragments = fragments;
        pipeline->firstBuffer = firstBuffer;
        pipeline->nextBuffer = nextBuffer;
        pipeline->malloc = malloc;

        perform(CreatingAssets);
    }

    // A copy engine can be used to populate buffers using GPU commands.
    void CreatingAssets(gObj<CopyingManager> manager) {
        // Copies a buffer written using an initializer_list
        manager gCopy   ListData(layerTransforms, {
                float3x3(
                    0,  0,  1,
                    0,  1,  0,
                    1,  0,  0),
                float3x3(
                    0,  0, -1,
                    0,  1,  0,
                   -1,  0,  0),
                float3x3(
                    1,  0,  0,
                    0,  0,  1,
                    0,  1,  0),
                float3x3(
                    1,  0,  0,
                    0,  0, -1,
                    0, -1,  0),
                float3x3(
                    1,  0,  0,
                    0, -1,  0,
                    0,  0,  1),
                float3x3(
                    1,  0,  0,
                    0,  1,  0,
                    0,  0, -1),
            });
    }

    void Graphics(gObj<GraphicsManager> manager) {
        sceneGeometry->ViewMatrix = ViewMatrix;
        ExecuteFrame(sceneGeometry);

        manager gClear  UAV(malloc, 0U);
        manager gClear  UAV(firstBuffer, (unsigned int)-1);
        manager gCopy   ValueData(screenInfo, ScreenInfo{ width, height });

        manager gSet Viewport(width, height);
        manager gSet Pipeline(pipeline);

        manager gDispatch Triangles(vertices->ElementCount);
    }

    void Frame() {
        perform(Graphics);
    }
};