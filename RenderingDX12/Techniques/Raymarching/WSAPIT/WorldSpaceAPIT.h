#pragma once
#include "../../../stdafx.h"
#include "../../Raymarching/RaymarchRT/Common_RaymarchRT.h"
#include "../../Common/CS_Constants.h"

class WorldSpaceTransformerPipeline : public ComputePipelineBindings {
public:
    gObj<Buffer> computeShaderInfo;

    // SRV
    gObj<Buffer> vertices;
    gObj<Buffer> objectIds;
    gObj<Buffer> transforms;

    // UAV
    gObj<Buffer> worldSpaceVertices;
    gObj<Buffer> sceneBoundaries;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WorldSpaceTransform_CS.cso"));
    }

    void Globals() {
        CBV(0, computeShaderInfo, ShaderType_Any);
        
        SRV(0, vertices, ShaderType_Any);
        SRV(1, objectIds, ShaderType_Any);
        SRV(2, transforms, ShaderType_Any);

        UAV(0, worldSpaceVertices, ShaderType_Any);
        UAV(1, sceneBoundaries, ShaderType_Any);
    }
};

class WorldSpaceABufferPipeline : public GraphicsPipelineBindings {
public:
    gObj<Buffer> screenInfo;

    gObj<Buffer> wsVertices;
    gObj<Buffer> sceneBoundaries;

    gObj<Buffer> fragments;
    gObj<Texture2D> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;

protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WSPIT_Fill_VS.cso"));
        _ gSet GeometryShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WSPIT_Fill_GS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WSPIT_Fill_PS.cso"));
    }

    void Globals() {
        SRV(0, wsVertices, ShaderType_Vertex);
        SRV(1, sceneBoundaries, ShaderType_Vertex);

        CBV(0, screenInfo, ShaderType_Geometry);

        CBV(0, screenInfo, ShaderType_Pixel);
        UAV(0, fragments, ShaderType_Pixel);
        UAV(1, firstBuffer, ShaderType_Pixel);
        UAV(2, nextBuffer, ShaderType_Pixel);
        UAV(3, malloc, ShaderType_Pixel);
    }
};

class WorldSpaceBuildPipeline : public ComputePipelineBindings {
public:
    // SRV
    gObj<Buffer> fragments;

    // UAV
    gObj<Texture2D> rootBuffer;
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> firstBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> malloc;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WSPIT_Build_CS.cso"));
    }

    void Globals() {
        SRV(0, fragments, ShaderType_Any);

        UAV(0, rootBuffer, ShaderType_Any);
        UAV(1, nodeBuffer, ShaderType_Any);
        UAV(2, boundaryNodeBuffer, ShaderType_Any);
        UAV(3, firstBuffer, ShaderType_Any);
        UAV(4, nextBuffer, ShaderType_Any);
        UAV(5, malloc, ShaderType_Any);
    }
};

class WorldSpaceLinkPipeline : public ComputePipelineBindings {
public:
    // SRV
    gObj<Texture2D> rootBuffer;

    // UAV
    gObj<Buffer> nodeBuffer;
    gObj<Buffer> boundaryNodeBuffer;
    gObj<Buffer> preorderBuffer;
    gObj<Buffer> skipBuffer;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WSPIT_Link_CS.cso"));
    }

    void Globals() {
        SRV(0, rootBuffer, ShaderType_Any);

        UAV(0, nodeBuffer, ShaderType_Any);
        UAV(1, boundaryNodeBuffer, ShaderType_Any);
        UAV(2, preorderBuffer, ShaderType_Any);
        UAV(3, skipBuffer, ShaderType_Any);
    }
};

class WorldSpaceTraversalPipeline : public ComputePipelineBindings {
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
    gObj<Texture2D> rootBuffer; // First buffer, then root buffer
    gObj<Buffer> nextBuffer; // NextBuffer, then per-node next buffer
    gObj<Buffer> preorderBuffer; // next node in preorder
    gObj<Buffer> skipBuffer; // next node in preorder skipping current subtree

    gObj<Buffer> hits;
    gObj<Texture2D> complexity; // Complexity buffer for debugging

    // Constant buffers
    gObj<Buffer> screenInfo;
    gObj<Buffer> raymarchingInfo;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\WSAPIT\\Shaders\\WSPIT_Traversal_CS.cso"));
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

        UAV(0, hits, ShaderType_Any);
        UAV(1, complexity, ShaderType_Any);

        CBV(0, screenInfo, ShaderType_Any);
        CBV(1, raymarchingInfo, ShaderType_Any);
    }
};

struct APITDescription {
	int Power;

	APITDescription() : APITDescription(8) {}
	APITDescription(int power) : Power(power) {}

	int getResolution() {
		return 1 << Power;
	}
};

class WorldSpaceAPIT : public Technique, public IHasScene, public DebugRaymarchRT {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<Buffer> Vertices;
    gObj<Buffer> SceneBoundaries;
    gObj<Buffer> Fragments;
    gObj<Texture2D> RootBuffer;
    gObj<Buffer> FirstBuffer;
    gObj<Buffer> NextBuffer;
    gObj<Buffer> Malloc;
    gObj<Buffer> screenInfo;
    gObj<Buffer> NodeBuffer;
    gObj<Buffer> BoundaryNodeBuffer;
    gObj<Buffer> PreorderBuffer;
    gObj<Buffer> SkipBuffer;
    gObj<Buffer> computeShaderInfo;

    gObj<WorldSpaceTransformerPipeline> wsTransformPipeline;
    gObj<WorldSpaceABufferPipeline> aBufferPipeline;
    gObj<WorldSpaceBuildPipeline> buildPipeline;
    gObj<WorldSpaceLinkPipeline> linkPipeline;
    gObj<WorldSpaceTraversalPipeline> traversalPipeline;

    APITDescription description;

    WorldSpaceAPIT(APITDescription description) {
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

        int resolution = description.getResolution();
        Vertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
        SceneBoundaries = _ gCreate RWStructuredBuffer<int3>(2);
        Fragments = _ gCreate RWStructuredBuffer<Fragment>(MAX_NUMBER_OF_FRAGMENTS);
        RootBuffer = _ gCreate DrawableTexture2D<int>(resolution, resolution);
        FirstBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        Malloc = _ gCreate RWStructuredBuffer<int>(1);
        screenInfo = _ gCreate ConstantBuffer<ScreenInfo>();
        NodeBuffer = _ gCreate RWStructuredBuffer<PITNode>(MAX_NUMBER_OF_FRAGMENTS);
        BoundaryNodeBuffer = _ gCreate RWStructuredBuffer<float4>(MAX_NUMBER_OF_FRAGMENTS);
        PreorderBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        SkipBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_FRAGMENTS);
        computeShaderInfo = _ gCreate ConstantBuffer<ComputeShaderInfo>();

        // Load and setup pipeline resources
        _ gLoad Pipeline(wsTransformPipeline);
        wsTransformPipeline->worldSpaceVertices = Vertices;
        wsTransformPipeline->sceneBoundaries = SceneBoundaries;
        wsTransformPipeline->vertices = sceneLoader->VertexBuffer;
        wsTransformPipeline->objectIds = sceneLoader->ObjectBuffer;
        wsTransformPipeline->transforms = sceneLoader->TransformBuffer;
        wsTransformPipeline->computeShaderInfo = computeShaderInfo;

        _ gLoad Pipeline(aBufferPipeline);
        aBufferPipeline->wsVertices = Vertices;
        aBufferPipeline->sceneBoundaries = SceneBoundaries;
        aBufferPipeline->fragments = Fragments;
        aBufferPipeline->firstBuffer = RootBuffer;
        aBufferPipeline->nextBuffer = NextBuffer;
        aBufferPipeline->malloc = Malloc;
        aBufferPipeline->screenInfo = screenInfo;

        _ gLoad Pipeline(buildPipeline);
        buildPipeline->fragments = Fragments;
        buildPipeline->rootBuffer = RootBuffer;
        buildPipeline->nodeBuffer = NodeBuffer;
        buildPipeline->boundaryNodeBuffer = BoundaryNodeBuffer;
        buildPipeline->firstBuffer = FirstBuffer;
        buildPipeline->nextBuffer = NextBuffer;
        buildPipeline->malloc = Malloc;

        _ gLoad Pipeline(linkPipeline);
        linkPipeline->rootBuffer = RootBuffer;
        linkPipeline->nodeBuffer = NodeBuffer;
        linkPipeline->boundaryNodeBuffer = BoundaryNodeBuffer;
        linkPipeline->preorderBuffer = PreorderBuffer;
        linkPipeline->skipBuffer = SkipBuffer;

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
    }

    void TransformVertices(gObj<GraphicsManager> manager) {
        manager gSet Pipeline(wsTransformPipeline);
        manager gCopy ValueData(computeShaderInfo, ComputeShaderInfo{ uint3(wsTransformPipeline->vertices->ElementCount, 0, 0) });
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_LINEARGROUP(wsTransformPipeline->vertices->ElementCount));
    }

    void BuildABuffer(gObj<GraphicsManager> manager) {
        int resolution = description.getResolution();
        manager gClear UAV(Malloc, 0U);
        manager gClear UAV(RootBuffer, (unsigned int)-1);
        manager gCopy ValueData(screenInfo, ScreenInfo{ resolution, resolution });

        manager gSet Viewport(resolution, resolution);
        manager gSet Pipeline(aBufferPipeline);
        
        manager gDispatch Triangles(aBufferPipeline->wsVertices->ElementCount);
    }

    void BuildPIT(gObj<GraphicsManager> manager) {
        int resolution = description.getResolution();

        manager gClear UAV(Malloc, 0U);
        manager gSet Pipeline(buildPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_SQUAREGROUP(resolution, resolution));

        manager gSet Pipeline(linkPipeline);
        manager.Dynamic_Cast<ComputeManager>() gDispatch Threads(CS_SQUAREGROUP(resolution, resolution));
    }

    void Frame() {
        perform(TransformVertices);
        perform(BuildABuffer);
        perform(BuildPIT);
    }
};