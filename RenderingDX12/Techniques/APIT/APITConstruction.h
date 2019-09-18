#pragma once

#include "ABuffer.h"
#include "../RaymarchRT/Common_RaymarchRT.h"

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

class TraverseAPIT_Pipeline : public ComputePipelineBindings {
public:
    gObj<Buffer> hits;
    gObj<Texture2D> complexity; // Complexity buffer for debugging

    gObj<Buffer> rays;
    gObj<Texture2D> rayHeadBuffer;
    gObj<Buffer> rayNextBuffer;

    gObj<Buffer> vertices;

    gObj<Buffer> fragments;

    gObj<Buffer> firstBuffer;
    gObj<Buffer> boundaryBuffer;
    gObj<Texture2D> rootBuffer; // First buffer, then root buffer
    gObj<Buffer> nextBuffer; // NextBuffer, then per-node next buffer
    gObj<Buffer> preorderBuffer; // next node in preorder
    gObj<Buffer> skipBuffer; // next node in preorder skipping current subtree
    gObj<Buffer> layerTransforms; // view transforms

    gObj<Buffer> morton;
    gObj<Buffer> startMipMaps;
    gObj<Buffer> mipMaps;

    // Constant buffers
    gObj<Buffer> screenInfo;
    gObj<Buffer> raymarchingInfo;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\APIT\\Shaders\\APITTraversal_CS.cso"));
    }

    void Globals() {
        CBV(0, screenInfo, ShaderType_Any);
        CBV(1, raymarchingInfo, ShaderType_Any);

        SRV(0, rays, ShaderType_Any);
        SRV(1, rayHeadBuffer, ShaderType_Any);
        SRV(2, rayNextBuffer, ShaderType_Any);

        SRV(3, vertices, ShaderType_Any);
        SRV(4, fragments, ShaderType_Any);
        SRV(5, firstBuffer, ShaderType_Any);
        SRV(6, boundaryBuffer, ShaderType_Any);
        SRV(7, rootBuffer, ShaderType_Any);
        SRV(8, nextBuffer, ShaderType_Any);
        SRV(9, preorderBuffer, ShaderType_Any);
        SRV(10, skipBuffer, ShaderType_Any);
        SRV(11, layerTransforms, ShaderType_Any);

        SRV(12, morton, ShaderType_Any);
        SRV(13, startMipMaps, ShaderType_Any);
        SRV(14, mipMaps, ShaderType_Any);

        UAV(0, hits, ShaderType_Any);
        UAV(1, complexity, ShaderType_Any);
    }
};

struct APITDescription {
    int Power;

    APITDescription() :APITDescription(8) {}
    APITDescription(int power) : Power(power) {}

    int getResolution() {
        return 1 << Power;
    }
};

class APITConstruction : public Technique, public IHasScene, public DebugRaymarchRT {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<ABuffer> aBuffer;
    
    gObj<Buffer> Vertices;
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
    gObj<TraverseAPIT_Pipeline> traversalPipeline;

    APITDescription description;
    float4x4 ViewMatrix;

    APITConstruction(APITDescription description) {
        this->description = description;
    }

    void ComputeCollision(gObj<GraphicsManager> manager) {
        manager gCopy ValueData(traversalPipeline->raymarchingInfo, RaymarchingDebug{
                    CountSteps ? 1 : 0,
                    CountHits ? 1 : 0
            });

        int width = traversalPipeline->rayHeadBuffer->Width;
        int height = traversalPipeline->rayHeadBuffer->Height;

        manager gSet Pipeline(traversalPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_SQUAREGROUP(width, height));
    }

    void DetectCollision(gObj<Buffer> rays, gObj<Texture2D> rayHeadBuffer, gObj<Buffer> rayNextBuffer, gObj<Buffer> hits, gObj<Texture2D> complexity)
    {
        traversalPipeline->rays = rays;
        traversalPipeline->rayHeadBuffer = rayHeadBuffer;
        traversalPipeline->rayNextBuffer = rayNextBuffer;
        traversalPipeline->hits = hits;
        traversalPipeline->complexity = complexity;

        perform(ComputeCollision);
    }

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
            x >>= 1;
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

        aBuffer = new ABuffer(description.getResolution(), description.getResolution(), description.Power + 1);
        aBuffer->sceneLoader = sceneLoader;
        _ gLoad Subprocess(aBuffer);

        // Load and setup pipeline resource
        _ gLoad Pipeline(buildPipeline);
        _ gLoad Pipeline(linkPipeline);
        _ gLoad Pipeline(initialMipPipeline);
        _ gLoad Pipeline(buildMipMapsPipeline);
        _ gLoad Pipeline(traversalPipeline);

        Vertices = aBuffer->Vertices;
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

        buildMipMapsPipeline->mipMaps = MipMaps;
        buildMipMapsPipeline->morton = Morton;
        buildMipMapsPipeline->startMipMaps = StartMipMaps;

        traversalPipeline->vertices = Vertices;
        traversalPipeline->fragments = Fragments;
        traversalPipeline->firstBuffer = FirstBuffer;
        traversalPipeline->boundaryBuffer = BoundaryBuffer;
        traversalPipeline->rootBuffer = RootBuffer;
        traversalPipeline->nextBuffer = NextBuffer;
        traversalPipeline->preorderBuffer = PreorderBuffer;
        traversalPipeline->skipBuffer = SkipBuffer;
        traversalPipeline->layerTransforms = aBuffer->layerTransforms;
        traversalPipeline->screenInfo = aBuffer->screenInfo;
        traversalPipeline->raymarchingInfo = _ gCreate ConstantBuffer<RaymarchingDebug>();
        traversalPipeline->morton = Morton;
        traversalPipeline->startMipMaps = StartMipMaps;
        traversalPipeline->mipMaps = MipMaps;

#ifndef USE_COMPUTESHADER
        perform(TestData);
#endif
    }

#ifndef USE_COMPUTESHADER
    gObj<Buffer> screenVertices;
    void TestData(gObj<CopyingManager> manager) {
        screenVertices = _ gCreate VertexBuffer<float2>(6);

        // Copies a buffer written using an initializer_list
        manager gCopy ListData(screenVertices, {
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
        int resolution = description.getResolution();
        Morton = _ gCreate StructuredBuffer<int>(resolution);

        int* morton = new int[resolution];
        for (int x = 0; x < resolution; x++)
            morton[x] = morton_index(x);
        manager gCopy PtrData(Morton, morton);
        //delete[] morton;

        int* startMipMaps = new int[description.Power + 1];
        int start = 0;
        int startResolution = resolution;
        for (int level = 0; level <= description.Power; level++)
        {
            startMipMaps[level] = start;
            start += startResolution * startResolution * 6;
            startResolution >>= 1;
        }
        StartMipMaps = _ gCreate StructuredBuffer<int>(description.Power + 1);
        manager gCopy PtrData(StartMipMaps, startMipMaps);
        MipMaps = _ gCreate RWStructuredBuffer<float2>(start);

        LevelInfos = new gObj<Buffer>[description.Power];
        for (int i = 0; i < description.Power; i++) {
            LevelInfos[i] = _ gCreate ConstantBuffer<LevelInfo>();
            manager gCopy ValueData(LevelInfos[i], LevelInfo{ resolution, i });
        }
    }

    void DrawScreen(gObj<GraphicsManager> manager, int width, int height) {
#ifdef USE_COMPUTESHADER
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_SQUAREGROUP(width, height));
#else
        manager gSet Viewport(width, height);
        manager gSet VertexBuffer(vertices);
        manager gDispatch Triangles(6);
#endif
    }

    void BuildMipMaps(gObj<GraphicsManager> manager) {
        int resolution = description.getResolution();
        manager gSet Pipeline(initialMipPipeline);
        DrawScreen(manager, resolution * 6, resolution);

        manager gSet Pipeline(buildMipMapsPipeline);
        for (int i = 1; i <= description.Power; i++)
        {
            buildMipMapsPipeline->levelInfo = LevelInfos[i - 1];
            DrawScreen(manager, 6 * (resolution >> i), resolution >> i);
        }
    }

    void Graphics(gObj<GraphicsManager> manager) {
        aBuffer->ViewMatrix = ViewMatrix;
        ExecuteFrame(aBuffer);

        int resolution = description.getResolution();

        manager gClear UAV(Malloc, 0U);
        manager gSet Pipeline(buildPipeline);
        DrawScreen(manager, resolution * 6, resolution);

        manager gSet Pipeline(linkPipeline);
        DrawScreen(manager, resolution * 6, resolution);
    }

    void Frame() {
        perform(Graphics);
        perform(BuildMipMaps);
    }
};