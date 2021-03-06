#pragma once
#include "../../../stdafx.h"
#include "../../Common/CS_Constants.h"

class WSMRAPIT_TransformerPipeline : public ComputePipelineBindings {
public:
    // CBV
    gObj<Buffer> computeShaderInfo;

    // SRV
    gObj<Buffer> vertices;
    gObj<Buffer> objectIds;
    gObj<Buffer> transforms;

    // UAV
    gObj<Buffer> worldSpaceVertices;
    gObj<Buffer> sceneBoundaries;
    gObj<Buffer> boundaries;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WorldSpaceTransform_CS.cso"));
    }

    void Globals() {
        CBV(0, computeShaderInfo, ShaderType_Any);

        SRV(0, vertices, ShaderType_Any);
        SRV(1, objectIds, ShaderType_Any);
        SRV(2, transforms, ShaderType_Any);

        UAV(0, worldSpaceVertices, ShaderType_Any);
        UAV(1, sceneBoundaries, ShaderType_Any);
        UAV(2, boundaries, ShaderType_Any);
    }
};

class WSMRAPIT_SceneBoundariesPipeline : public ComputePipelineBindings {
public:
    // CBV
    gObj<Buffer> computeShaderInfo;

    // UAV
    gObj<Buffer> boundaries;
    gObj<Buffer> sceneBoundaries;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\ComputeSceneBoundaries_CS.cso"));
    }

    void Globals() {
        CBV(0, computeShaderInfo, ShaderType_Any);

        UAV(0, boundaries, ShaderType_Any);
        //UAV(1, sceneBoundaries, ShaderType_Any);
    }
};

class WSMRAPIT_ABufferPipeline : public GraphicsPipelineBindings {
public:
    gObj<Buffer> screenInfo;
    gObj<Buffer> reticulation;

    gObj<Buffer> wsVertices;
    gObj<Buffer> sceneBoundaries;
    gObj<Buffer> boundaries;

    gObj<Buffer> fragments;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;

protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WSMRPIT_Fill_VS.cso"));
        _ gSet GeometryShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WSMRPIT_Fill_GS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WSMRPIT_Fill_PS.cso"));
    }

    void Globals() {
        SRV(0, wsVertices, ShaderType_Vertex);
        SRV(1, sceneBoundaries, ShaderType_Vertex);
        SRV(2, boundaries, ShaderType_Vertex);

        CBV(0, screenInfo, ShaderType_Geometry);
        CBV(1, reticulation, ShaderType_Geometry);

        CBV(0, screenInfo, ShaderType_Pixel);
        UAV(0, fragments, ShaderType_Pixel);
        UAV(1, firstBuffer, ShaderType_Pixel);
        UAV(2, nextBuffer, ShaderType_Pixel);
        UAV(3, malloc, ShaderType_Pixel);
    }
};

class WSMRAPIT_BuildPipeline : public ComputePipelineBindings {
public:
    // CBV
    gObj<Buffer> computeShaderInfo;

    // SRV
    gObj<Buffer> fragments;

    // UAV
    gObj<Buffer> rootBuffer;
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WSMRPIT_Build_CS.cso"));
    }

    void Globals() {
        CBV(0, computeShaderInfo, ShaderType_Any);

        SRV(0, fragments, ShaderType_Any);

        UAV(0, rootBuffer, ShaderType_Any);
        UAV(1, nodeBuffer, ShaderType_Any);
        UAV(2, boundaryNodeBuffer, ShaderType_Any);
        UAV(3, firstBuffer, ShaderType_Any);
        UAV(4, nextBuffer, ShaderType_Any);
        UAV(5, malloc, ShaderType_Any);
    }
};

class WSMRAPIT_LinkPipeline : public ComputePipelineBindings {
public:
    // CBV
    gObj<Buffer> computeShaderInfo;

    // SRV
    gObj<Buffer> rootBuffer;

    // UAV
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> preorderBuffer;
    gObj<Buffer> skipBuffer;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WSMRPIT_Link_CS.cso"));
    }

    void Globals() {
        CBV(0, computeShaderInfo, ShaderType_Any);

        SRV(0, rootBuffer, ShaderType_Any);

        UAV(0, nodeBuffer, ShaderType_Any);
        UAV(1, boundaryNodeBuffer, ShaderType_Any);
        UAV(2, preorderBuffer, ShaderType_Any);
        UAV(3, skipBuffer, ShaderType_Any);
    }
};

class WSMRAPIT_TraversalPipeline : public ComputePipelineBindings {
public:
    // SRV
    gObj<Buffer> rays;
    gObj<Texture2D> rayHeadBuffer;
    gObj<Buffer> rayNextBuffer;
    // Scene Geometry
    gObj<Buffer> wsVertices;
    gObj<Buffer> sceneBoundaries;
    // APIT
    gObj<Buffer> fragments;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> rootBuffer; // First buffer, then root buffer
    gObj<Buffer> nextBuffer; // NextBuffer, then per-node next buffer
    gObj<Buffer> preorderBuffer; // next node in preorder
    gObj<Buffer> skipBuffer; // next node in preorder skipping current subtree

    gObj<Buffer> boundaries;

    gObj<Buffer> hits;
    gObj<Texture2D> complexity; // Complexity buffer for debugging

    // Constant buffers
    gObj<Buffer> screenInfo;
    gObj<Buffer> raymarchingInfo;
    gObj<Buffer> computeShaderInfo;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WS_MRAPIT\\Shaders\\WSMRPIT_Traversal_CS.cso"));
    }

    void Globals() {
        SRV(0, rays, ShaderType_Any);
        SRV(1, rayHeadBuffer, ShaderType_Any);
        SRV(2, rayNextBuffer, ShaderType_Any);
        SRV(3, wsVertices, ShaderType_Any);
        SRV(4, sceneBoundaries, ShaderType_Any);
        SRV(5, fragments, ShaderType_Any);
        SRV(6, firstBuffer, ShaderType_Any);
        SRV(7, boundaryNodeBuffer, ShaderType_Any);
        SRV(8, rootBuffer, ShaderType_Any);
        SRV(9, nextBuffer, ShaderType_Any);
        SRV(10, preorderBuffer, ShaderType_Any);
        SRV(11, skipBuffer, ShaderType_Any);
        SRV(12, boundaries, ShaderType_Any);

        UAV(0, hits, ShaderType_Any);
        UAV(1, complexity, ShaderType_Any);

        CBV(0, screenInfo, ShaderType_Any);
        CBV(1, raymarchingInfo, ShaderType_Any);
        CBV(2, computeShaderInfo, ShaderType_Any);
    }
};

struct MRAPITDescription {
    int Delta;
    int K;
    int Power;

    MRAPITDescription() : MRAPITDescription(4, 2, 8) {}
    MRAPITDescription(int k, int delta, int power) : K(k), Delta(delta), Power(power) {}

    int getResolution() {
        return 1 << Power;
    }

    int countEntries() {
        return ((1 << ((Power + 1) * 2)) - 1) / 3;
    }
};

class WorldSpaceMRAPIT : public Technique, public IHasScene, public DebugRaymarchRT {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<Buffer> Vertices;
    gObj<Buffer> SceneBoundaries;
    gObj<Buffer> Fragments;
    gObj<Buffer> RootBuffer;
    gObj<Buffer> FirstBuffer;
    gObj<Buffer> NextBuffer;
    gObj<Buffer> Malloc;
    gObj<Buffer> screenInfo;
    gObj<Buffer> NodeBuffer;
    gObj<Buffer> BoundaryNodeBuffer;
    gObj<Buffer> PreorderBuffer;
    gObj<Buffer> SkipBuffer;
    gObj<Buffer> reticulation;
    gObj<Buffer> apitComputeShaderInfo;

    gObj<Buffer> boundaries;

    gObj<WSMRAPIT_TransformerPipeline> wsTransformPipeline;
    gObj<WSMRAPIT_SceneBoundariesPipeline> sceneBoundariesPipeline;

    gObj<WSMRAPIT_ABufferPipeline> aBufferPipeline;
    gObj<WSMRAPIT_BuildPipeline> buildPipeline;
    gObj<WSMRAPIT_LinkPipeline> linkPipeline;
    gObj<WSMRAPIT_TraversalPipeline> traversalPipeline;

    MRAPITDescription description;

    WorldSpaceMRAPIT(MRAPITDescription description) {
        this->description = description;
    }

    void ComputeCollision(gObj<GraphicsManager> manager)
    {
        manager gCopy ValueData(traversalPipeline->raymarchingInfo, RaymarchingDebug{
            CountSteps ? 1 : 0,
            CountHits ? 1 : 0
            });

        int width = traversalPipeline->rayHeadBuffer->Width;
        int height = traversalPipeline->rayHeadBuffer->Height;
        
        manager gCopy ValueData(traversalPipeline->computeShaderInfo, ComputeShaderInfo{ uint3(width, height, 0) });
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

    void Startup() {
        // Load scene in retained mode
        if (sceneLoader == nullptr) {
            sceneLoader = new RetainedSceneLoader();
            sceneLoader->SetScene(Scene);
            _ gLoad Subprocess(sceneLoader);
        }

        int totalEntries = description.countEntries();
        Vertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
        SceneBoundaries = _ gCreate RWStructuredBuffer<int3>(2);
        Fragments = _ gCreate RWStructuredBuffer<Fragment>(MAX_NUMBER_OF_FRAGMENTS);
        RootBuffer = _ gCreate RWStructuredBuffer<int>(totalEntries);
        FirstBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        Malloc = _ gCreate RWStructuredBuffer<int>(1);
        screenInfo = _ gCreate ConstantBuffer<ScreenInfo>();
        NodeBuffer = _ gCreate RWStructuredBuffer<PITNode>(MAX_NUMBER_OF_FRAGMENTS);
        BoundaryNodeBuffer = _ gCreate RWStructuredBuffer<float4>(MAX_NUMBER_OF_FRAGMENTS);
        PreorderBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        SkipBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        reticulation = _ gCreate ConstantBuffer<Reticulation>();
        apitComputeShaderInfo = _ gCreate ConstantBuffer<ComputeShaderInfo>();
        boundaries = _ gCreate RWStructuredBuffer<MinMax>(sceneLoader->VertexBuffer->ElementCount);

        // Load and setup pipeline resources
        _ gLoad Pipeline(wsTransformPipeline);
        wsTransformPipeline->worldSpaceVertices = Vertices;
        wsTransformPipeline->sceneBoundaries = SceneBoundaries;
        wsTransformPipeline->vertices = sceneLoader->VertexBuffer;
        wsTransformPipeline->objectIds = sceneLoader->ObjectBuffer;
        wsTransformPipeline->transforms = sceneLoader->TransformBuffer;
        wsTransformPipeline->computeShaderInfo = _ gCreate ConstantBuffer<ComputeShaderInfo>();
        wsTransformPipeline->boundaries = boundaries;

        _ gLoad Pipeline(sceneBoundariesPipeline);
        sceneBoundariesPipeline->computeShaderInfo = wsTransformPipeline->computeShaderInfo;
        sceneBoundariesPipeline->boundaries = boundaries;
        sceneBoundariesPipeline->sceneBoundaries = SceneBoundaries;

        _ gLoad Pipeline(aBufferPipeline);
        aBufferPipeline->wsVertices = Vertices;
        aBufferPipeline->sceneBoundaries = SceneBoundaries;
        aBufferPipeline->fragments = Fragments;
        aBufferPipeline->firstBuffer = RootBuffer;
        aBufferPipeline->nextBuffer = NextBuffer;
        aBufferPipeline->malloc = Malloc;
        aBufferPipeline->screenInfo = screenInfo;
        aBufferPipeline->reticulation = reticulation;
        aBufferPipeline->boundaries = boundaries;

        _ gLoad Pipeline(buildPipeline);
        buildPipeline->fragments = Fragments;
        buildPipeline->rootBuffer = RootBuffer;
        buildPipeline->nodeBuffer = NodeBuffer;
        buildPipeline->boundaryNodeBuffer = BoundaryNodeBuffer;
        buildPipeline->firstBuffer = FirstBuffer;
        buildPipeline->nextBuffer = NextBuffer;
        buildPipeline->malloc = Malloc;
        buildPipeline->computeShaderInfo = apitComputeShaderInfo;

        _ gLoad Pipeline(linkPipeline);
        linkPipeline->rootBuffer = RootBuffer;
        linkPipeline->nodeBuffer = NodeBuffer;
        linkPipeline->boundaryNodeBuffer = BoundaryNodeBuffer;
        linkPipeline->preorderBuffer = PreorderBuffer;
        linkPipeline->skipBuffer = SkipBuffer;
        linkPipeline->computeShaderInfo = apitComputeShaderInfo;

        _ gLoad Pipeline(traversalPipeline);
        traversalPipeline->wsVertices = Vertices;
        traversalPipeline->sceneBoundaries = SceneBoundaries;
        traversalPipeline->fragments = Fragments;
        traversalPipeline->firstBuffer = FirstBuffer;
        traversalPipeline->boundaryNodeBuffer = BoundaryNodeBuffer;
        traversalPipeline->rootBuffer = RootBuffer;
        traversalPipeline->nextBuffer = NextBuffer;
        traversalPipeline->preorderBuffer = PreorderBuffer;
        traversalPipeline->skipBuffer = SkipBuffer;
        traversalPipeline->screenInfo = screenInfo;
        traversalPipeline->raymarchingInfo = _ gCreate ConstantBuffer<RaymarchingDebug>();
        traversalPipeline->computeShaderInfo = _ gCreate ConstantBuffer<ComputeShaderInfo>();
        traversalPipeline->boundaries = boundaries;
    }

    void TransformVertices(gObj<GraphicsManager> manager) {
        manager gSet Pipeline(wsTransformPipeline);
        manager gCopy ValueData(wsTransformPipeline->computeShaderInfo, ComputeShaderInfo{ uint3(wsTransformPipeline->vertices->ElementCount, 0, 0) });
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_LINEARGROUP(wsTransformPipeline->vertices->ElementCount));

        int k = (int)(ceil(wsTransformPipeline->vertices->ElementCount * 1.0 / CS_GROUPSIZE_1D));
        while (k > 0) {
            manager gSet Pipeline(sceneBoundariesPipeline);
            manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(k);

            k >>= 1;
        }

        /*manager gSet Pipeline(sceneBoundariesPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(k);*/
    }

    void BuildABuffer(gObj<GraphicsManager> manager) {
        int resolution = description.getResolution();
        manager gClear UAV(Malloc, 0U);
        manager gClear UAV(RootBuffer, (unsigned int)-1);
        manager gCopy ValueData(screenInfo, ScreenInfo{ resolution, resolution, description.Power });
        manager gCopy ValueData(reticulation, Reticulation{ description.K, description.Delta });

        manager gSet Viewport(resolution, resolution);
        manager gSet Pipeline(aBufferPipeline);

        manager gDispatch Triangles(aBufferPipeline->wsVertices->ElementCount);
    }

    void BuildPIT(gObj<GraphicsManager> manager) {
        int entries = description.countEntries();

        manager gCopy ValueData(apitComputeShaderInfo, ComputeShaderInfo{ uint3(entries, 0, 0) });
        manager gClear UAV(Malloc, 0U);
        manager gSet Pipeline(buildPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_LINEARGROUP(entries));

        manager gSet Pipeline(linkPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_LINEARGROUP(entries));
    }

    void Frame() {
        perform(TransformVertices);
        perform(BuildABuffer);
        perform(BuildPIT);
    }
};