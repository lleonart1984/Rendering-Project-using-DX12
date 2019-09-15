#pragma once

#include "ABuffer.h"

class BuildAPIT_Pipeline :
#ifdef USE_COMPUTESHADER
    public ComputePipelineBindings {
    ShaderType type = ShaderType_Any;
#else
    public ShowTexturePipeline{
    ShaderType type = ShaderType_Pixel;
#endif
public:
    // SRV
    gObj<Buffer> fragments;

    // UAV
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryBuffer;
    gObj<Texture2D> rootBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;
    gObj<Buffer> depth;
protected:
    void Setup() {
#ifdef USE_COMPUTESHADER
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_Build_CS.cso"));
#else
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_Build_PS.cso"));
#endif
    }

    void Globals() {
        SRV(0, fragments, type);

        UAV(0, firstBuffer, type);
        UAV(1, nodeBuffer, type);
        UAV(2, boundaryBuffer, type);
        UAV(3, rootBuffer, type);
        UAV(4, nextBuffer, type);
        UAV(5, malloc, type);
        UAV(6, depth, type);
    }
};

class LinkAPIT_Pipeline :
#ifdef USE_COMPUTESHADER
    public ComputePipelineBindings {
    ShaderType type = ShaderType_Any;
#else
    public ShowTexturePipeline{
    ShaderType type = ShaderType_Pixel;
#endif
public:
    // SRV
    gObj<Texture2D> rootBuffer;

    // UAV
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryBuffer;
    gObj<Buffer> preorderBuffer;
    gObj<Buffer> skipBuffer;
protected:
    void Setup() {
#ifdef USE_COMPUTESHADER
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_Link_CS.cso"));
#else
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_Link_PS.cso"));
#endif
    }

    void Globals() {
        SRV(0, rootBuffer, type);

        UAV(0, nodeBuffer, type);
        UAV(1, boundaryBuffer, type);
        UAV(2, preorderBuffer, type);
        UAV(3, skipBuffer, type);
    }
};

class InitialMipAPIT_Pipeline :
#ifdef USE_COMPUTESHADER
    public ComputePipelineBindings {
    ShaderType type = ShaderType_Any;
#else
    public ShowTexturePipeline{
    ShaderType type = ShaderType_Pixel;
#endif
public:
    // SRV
    gObj<Texture2D> rootBuffer;
    gObj<Buffer> boundaryBuffer;
    gObj<Buffer> morton;

    // UAV
    gObj<Buffer> mipMaps;
protected:
    void Setup() {
#ifdef USE_COMPUTESHADER
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_InitialMip_CS.cso"));
#else
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_InitialMip_PS.cso"));
#endif
    }

    void Globals() {
        SRV(0, rootBuffer, type);
        SRV(1, boundaryBuffer, type);
        SRV(2, morton, type);

        UAV(0, mipMaps, type);
    }
};

class BuildMipsAPIT_Pipeline :
#ifdef USE_COMPUTESHADER
    public ComputePipelineBindings {
    ShaderType type = ShaderType_Any;
#else
    public ShowTexturePipeline{
    ShaderType type = ShaderType_Pixel;
#endif
public:
    // SRV
    gObj<Buffer> levelInfo;
    gObj<Buffer> morton;
    gObj<Buffer> startMipMaps;

    // UAV
    gObj<Buffer> mipMaps;
protected:
    void Setup() {
#ifdef USE_COMPUTESHADER
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_BuildMips_CS.cso"));
#else
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\CPIT_BuildMips_PS.cso"));
#endif
    }

    void Globals() {
        SRV(0, morton, type);
        SRV(1, startMipMaps, type);

        UAV(0, mipMaps, type);
    }

    void Locals() {
        CBV(0, levelInfo, type);
    }
};

class APITConstruction : public Technique, public IHasScene {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<ABuffer> aBuffer;

    gObj<Buffer> Fragments;
    gObj<Buffer> FirstBuffer;
    gObj<Buffer> NodeBuffer;
    gObj<Buffer> BoundaryBuffer;
    gObj<Texture2D> RootBuffer;
    gObj<Buffer> NextBuffer;
    gObj<Buffer> Malloc;
    gObj<Buffer> Depth;
    gObj<Buffer> PreorderBuffer;
    gObj<Buffer> SkipBuffer;

    gObj<Buffer> MipMaps;
    gObj<Buffer> Morton;
    gObj<Buffer> StartMipMaps;
    gObj<Buffer>* LevelInfos;

    gObj<BuildAPIT_Pipeline> buildPipeline;
    gObj<LinkAPIT_Pipeline> linkPipeline;
    gObj<InitialMipAPIT_Pipeline> initialMipPipeline;
    gObj<BuildMipsAPIT_Pipeline> buildMipMapsPipeline;

    int Power;
    float4x4 ViewMatrix;

    int getResolution() {
        return 1 << Power;
    }

    APITConstruction(int power)
        : Power(power) {}

protected:
    void SetScene(gObj<CA4G::Scene> scene) {
        IHasScene::SetScene(scene);
        if (sceneLoader != nullptr) {
            sceneLoader->SetScene(scene);
        }
    }

    int morton_index(int x) {
        int total = 0;
        int bit = 0;
        while (x != 0)
        {
            total = total + ((x % 2) << (2 * bit));
            bit++;
            x /= 2;
        }
        return total;
    }

    void Startup() {
        // Load scene in retained mode
        if (sceneLoader == nullptr) {
            sceneLoader = new RetainedSceneLoader();
            sceneLoader->SetScene(Scene);
            _ gLoad Subprocess(sceneLoader);
        }

        aBuffer = new ABuffer(getResolution(), getResolution());
        aBuffer->sceneLoader = sceneLoader;
        _ gLoad Subprocess(aBuffer);

        // Load and setup pipeline resource
        _ gLoad Pipeline(buildPipeline);
        _ gLoad Pipeline(linkPipeline);
        _ gLoad Pipeline(initialMipPipeline);
        _ gLoad Pipeline(buildMipMapsPipeline);

        Fragments = aBuffer->Fragments;
        FirstBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        NodeBuffer = _ gCreate RWStructuredBuffer<PITNode>(MAX_NUMBER_OF_FRAGMENTS);
        BoundaryBuffer = _ gCreate RWStructuredBuffer<float4>(MAX_NUMBER_OF_FRAGMENTS);
        RootBuffer = aBuffer->FirstBuffer;
        NextBuffer = aBuffer->NextBuffer;
        Malloc = aBuffer->Malloc;
        Depth = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        PreorderBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        SkipBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        perform(CreatingAssets);

        buildPipeline->fragments = Fragments;
        buildPipeline->firstBuffer = FirstBuffer;
        buildPipeline->nodeBuffer = NodeBuffer;
        buildPipeline->boundaryBuffer = BoundaryBuffer;
        buildPipeline->rootBuffer = RootBuffer;
        buildPipeline->nextBuffer = NextBuffer;
        buildPipeline->malloc = Malloc;
        buildPipeline->depth = Depth;

        linkPipeline->rootBuffer = RootBuffer;
        linkPipeline->nodeBuffer = NodeBuffer;
        linkPipeline->boundaryBuffer = BoundaryBuffer;
        linkPipeline->preorderBuffer = PreorderBuffer;
        linkPipeline->skipBuffer = SkipBuffer;

        initialMipPipeline->rootBuffer = RootBuffer;
        initialMipPipeline->mipMaps = MipMaps;
        initialMipPipeline->morton = Morton;
        initialMipPipeline->boundaryBuffer = BoundaryBuffer;

        //buildMipMapsPipeline->levelInfo = _ gCreate ConstantBuffer<LevelInfo>();
        buildMipMapsPipeline->mipMaps = MipMaps;
        buildMipMapsPipeline->morton = Morton;
        buildMipMapsPipeline->startMipMaps = StartMipMaps;

#ifndef USE_COMPUTESHADER
        perform(TestData);
#endif
    }

#ifndef USE_COMPUTESHADER
    gObj<Buffer> vertices;
    void TestData(gObj<CopyingManager> manager) {
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
#endif

    void CreatingAssets(gObj<CopyingManager> manager) {
        int resolution = getResolution();
        Morton = _ gCreate StructuredBuffer<int>(resolution);

        int* morton = new int[resolution];
        for (int x = 0; x < resolution; x++)
            morton[x] = morton_index(x);
        manager gCopy PtrData(Morton, morton);
        //delete[] morton;

        int* startMipMaps = new int[Power + 1];
        int start = 0;
        for (int level = 0; level <= Power; level++)
        {
            startMipMaps[level] = start;
            start += resolution * resolution * 6;
            resolution /= 2;
        }
        StartMipMaps = _ gCreate StructuredBuffer<int>(Power + 1);
        manager gCopy PtrData(StartMipMaps, startMipMaps);
        MipMaps = _ gCreate RWStructuredBuffer<float2>(start);

        LevelInfos = new gObj<Buffer>[Power];
        for (int i = 0; i < Power; i++) {
            LevelInfos[i] = _ gCreate ConstantBuffer<LevelInfo>();
            manager gCopy ValueData(LevelInfos[i], LevelInfo{ resolution, i });
        }
    }

    void DrawScreen(gObj<GraphicsManager> manager, gObj<IPipelineBindings> pipeline, int width, int height) {
#ifdef USE_COMPUTESHADER
        manager gSet Pipeline(pipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(width, height);
#else
        manager gSet Viewport(width, height);
        manager gSet Pipeline(pipeline);
        manager gSet VertexBuffer(vertices);
        manager gDispatch Triangles(6);
#endif
    }

    void BuildMipMaps(gObj<GraphicsManager> manager) {
        int resolution = getResolution();
        DrawScreen(manager, initialMipPipeline, resolution * 6, resolution);

        for (int i = 1; i <= Power; i++)
        {
            buildMipMapsPipeline->levelInfo = LevelInfos[i - 1];
            DrawScreen(manager, buildMipMapsPipeline, 6 * (resolution >> i), resolution >> i);
        }
    }

    void Graphics(gObj<GraphicsManager> manager) {
        aBuffer->ViewMatrix = ViewMatrix;
        ExecuteFrame(aBuffer);

        int resolution = getResolution();

        manager gClear UAV(Malloc, 0U);
        DrawScreen(manager, buildPipeline, resolution * 6, resolution);

        DrawScreen(manager, linkPipeline, resolution * 6, resolution);
    }

    void Frame() {
        perform(Graphics);
        perform(BuildMipMaps);
    }
};