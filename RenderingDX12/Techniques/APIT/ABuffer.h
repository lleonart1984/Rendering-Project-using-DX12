#pragma once

#include "ViewSpaceTransformer.h"

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
    gObj<ViewSpaceTransformer> viewSpaceTransformer;

    gObj<Buffer> Fragments;
    gObj<Texture2D> FirstBuffer;
    gObj<Buffer> NextBuffer;
    gObj<Buffer> Malloc;

    gObj<Buffer> screenInfo;
    gObj<Buffer> vertices;
    gObj<Buffer> layerTransforms;

    const int Width, Height;
    float4x4 ViewMatrix;

    gObj<ABufferPipeline> pipeline;

    ABuffer(int width, int height)
        : Width(width), Height(height) { }

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

        viewSpaceTransformer = new ViewSpaceTransformer();
        viewSpaceTransformer->sceneLoader = sceneLoader;
        _ gLoad Subprocess(viewSpaceTransformer);

        // Load and setup pipeline resource
        _ gLoad Pipeline(pipeline);

        vertices = viewSpaceTransformer->ViewSpaceVertices;
        Fragments = _ gCreate RWStructuredBuffer<Fragment>(MAX_NUMBER_OF_FRAGMENTS);
        FirstBuffer = _ gCreate DrawableTexture2D<int>(Width * 6, Height);
        NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        Malloc = _ gCreate RWStructuredBuffer<int>(1);

        // Create globals VS constant buffer
        screenInfo = _ gCreate ConstantBuffer<ScreenInfo>();
        layerTransforms = _ gCreate StructuredBuffer<float3x3>(6);
        pipeline->screenInfo = screenInfo;
        pipeline->layerTransforms = layerTransforms;

        pipeline->vertices = vertices;
        pipeline->fragments = Fragments;
        pipeline->firstBuffer = FirstBuffer;
        pipeline->nextBuffer = NextBuffer;
        pipeline->malloc = Malloc;

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
        viewSpaceTransformer->ViewMatrix = ViewMatrix;
        ExecuteFrame(viewSpaceTransformer);

        manager gClear  UAV(Malloc, 0U);
        manager gClear  UAV(FirstBuffer, (unsigned int)-1);
        manager gCopy   ValueData(screenInfo, ScreenInfo{ Width, Height });

        manager gSet Viewport(Width, Height);
        manager gSet Pipeline(pipeline);

        manager gDispatch Triangles(vertices->ElementCount);
    }

    void Frame() {
        perform(Graphics);
    }
};