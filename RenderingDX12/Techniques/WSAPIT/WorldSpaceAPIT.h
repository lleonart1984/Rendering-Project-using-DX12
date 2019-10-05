#pragma once
#include "../../stdafx.h"
#include "../Common/CS_Constants.h"

class WorldSpaceTransformerPipeline : public ComputePipelineBindings {
public:
    // SRV
    gObj<Buffer> vertices;
    gObj<Buffer> objectIds;
    gObj<Buffer> transforms;

    // UAV
    gObj<Buffer> worldSpaceVertices;
    gObj<Buffer> sceneBoundaries;
protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WorldSpaceTransform_CS.cso"));
    }

    void Globals() {
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
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Fill_VS.cso"));
        _ gSet GeometryShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Fill_GS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Fill_PS.cso"));
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
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Build_CS.cso"));
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
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\WSAPIT\\Shaders\\WSPIT_Link_CS.cso"));
    }

    void Globals() {
        SRV(0, rootBuffer, ShaderType_Any);

        UAV(0, nodeBuffer, ShaderType_Any);
        UAV(1, boundaryNodeBuffer, ShaderType_Any);
        UAV(2, preorderBuffer, ShaderType_Any);
        UAV(3, skipBuffer, ShaderType_Any);
    }
};

class WorldSpaceAPIT : public Technique, public IHasScene {
public:
    // Scene loading process to retain scene on the GPU
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<Buffer> WorldSpaceVertices;
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

    gObj<WorldSpaceTransformerPipeline> wsTransformPipeline;
    gObj<WorldSpaceABufferPipeline> aBufferPipeline;
    gObj<WorldSpaceBuildPipeline> buildPipeline;
    gObj<WorldSpaceLinkPipeline> linkPipeline;

    APITDescription description;

    WorldSpaceAPIT(APITDescription description) {
        this->description = description;
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
        WorldSpaceVertices = _ gCreate RWStructuredBuffer<SCENE_VERTEX>(sceneLoader->VertexBuffer->ElementCount);
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

        // Load and setup pipeline resources
        _ gLoad Pipeline(wsTransformPipeline);
        wsTransformPipeline->worldSpaceVertices = WorldSpaceVertices;
        wsTransformPipeline->sceneBoundaries = SceneBoundaries;
        wsTransformPipeline->vertices = sceneLoader->VertexBuffer;
        wsTransformPipeline->objectIds = sceneLoader->ObjectBuffer;
        wsTransformPipeline->transforms = sceneLoader->TransformBuffer;

        _ gLoad Pipeline(aBufferPipeline);
        aBufferPipeline->wsVertices = WorldSpaceVertices;
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
    }

    void TransformVertices(gObj<GraphicsManager> manager) {
        manager gSet Pipeline(wsTransformPipeline);
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