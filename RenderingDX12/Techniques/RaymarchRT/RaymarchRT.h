#pragma once

#include "../../stdafx.h"
#include "RaymarchRT_Constants.h"

#define BOUNCES 2

class InitializeRaysPipeline : public GraphicsPipelineBindings {
public:
    gObj<Buffer> cameraCB;
    gObj<Buffer> vertices;
    gObj<Texture2D> initialTriangleIndices;
    gObj<Texture2D> initialCoordinates;
    gObj<Texture2D> depthBuffer;

protected:
    void Setup() {
        _ gSet InputLayout({});
        _ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\RaymarchRT\\Shaders\\RT_InitialRays_VS.cso"));
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\RaymarchRT\\Shaders\\RT_InitialRays_PS.cso"));
        _ gSet DepthTest();
    }

    void Globals() {
        // VS
        CBV(0, cameraCB, ShaderType_Vertex);
        SRV(0, vertices, ShaderType_Vertex);

        // PS
        SRV(0, vertices, ShaderType_Pixel);
        RTV(0, initialTriangleIndices);
        RTV(1, initialCoordinates);
        DBV(depthBuffer);
    }
};

class InitializeRayDirectionsPipeline : public ShowTexturePipeline {
public:
    gObj<Buffer> inverseCamera;
    gObj<Buffer> screenInfo;
    gObj<Texture2D> initialTriangleIndices;
    gObj<Texture2D> initialCoordinates;
    gObj<Buffer> rayInfo;
    gObj<Buffer> rayIntersections;
    gObj<Texture2D> headBuffer;
    gObj<Buffer> nextBuffer;

protected:
    void Setup() {
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\RaymarchRT\\Shaders\\RT_InitialRayDirections_PS.cso"));
    }

    void Globals() {
        CBV(0, inverseCamera, ShaderType_Pixel);
        CBV(1, screenInfo, ShaderType_Pixel);

        SRV(0, initialTriangleIndices, ShaderType_Pixel);
        SRV(1, initialCoordinates, ShaderType_Pixel);
        UAV(0, rayInfo, ShaderType_Pixel);
        UAV(1, rayIntersections, ShaderType_Pixel);
        UAV(2, headBuffer, ShaderType_Pixel);
        UAV(3, nextBuffer, ShaderType_Pixel);
    }
};

class BounceRaysPipeline : public ShowTexturePipeline {
public:
    // CBs
    gObj<Buffer> lighting;
    gObj<Buffer> inverseViewing;

    // SRVs
    gObj<Buffer> input;
    gObj<Texture2D> headBuffer;
    gObj<Buffer> nextBuffer;
    gObj<Buffer> intersections;

    gObj<Buffer> vertices;
    gObj<Buffer> objectBuffer;
    gObj<Buffer> materialIndexBuffer;
    gObj<Buffer> materials;

    gObj<Texture2D>* textures = nullptr;
    int textureCount;

    // UAVs
    gObj<Buffer> output;
    gObj<Texture2D> outputHeadBuffer;
    gObj<Buffer> outputNextBuffer;
    gObj<Buffer> malloc;

    // RT
    gObj<Texture2D> accumulations;

protected:
    void Setup() {
        ShowTexturePipeline::Setup();
        _ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\RaymarchRT\\Shaders\\RT_Bounce_PS.cso"));
        //_ gSet AllRenderTargets(1, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
        //_ gSet IndependentBlend(true);
        //_ gSet AlphaToCoverage(true);
        //_ gSet BlendSampleMask(0);
        _ gSet BlendForAllRenderTargets(true, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ONE);
    }

    void Globals() {
        RTV(0, accumulations);

        CBV(0, lighting, ShaderType_Pixel);
        CBV(1, inverseViewing, ShaderType_Pixel);

        SRV(4, vertices, ShaderType_Pixel);
        SRV(5, objectBuffer, ShaderType_Pixel);
        SRV(6, materialIndexBuffer, ShaderType_Pixel);
        SRV(7, materials, ShaderType_Pixel);
        SRV_Array(8, textures, textureCount, ShaderType_Pixel);
        Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);
    }

    void Locals() {
        SRV(0, input, ShaderType_Pixel);
        SRV(1, headBuffer, ShaderType_Pixel);
        SRV(2, nextBuffer, ShaderType_Pixel);
        SRV(3, intersections, ShaderType_Pixel);

        UAV(1, output, ShaderType_Pixel);
        UAV(2, outputHeadBuffer, ShaderType_Pixel);
        UAV(3, outputNextBuffer, ShaderType_Pixel);
        UAV(4, malloc, ShaderType_Pixel);
    }
};

template<typename C, typename D>
class RaymarchRT : public Technique, public IHasScene, public IHasCamera, public IHasLight, public IHasBackcolor, public IHasRaymarchDebugInfo {
public:
    gObj<RetainedSceneLoader> sceneLoader;
    gObj<C> constructing;
    D constructingDescription;

    gObj<Buffer> Rays;
    gObj<Texture2D> RayHeadBuffer;
    gObj<Buffer> RayNextBuffer;
    gObj<Buffer> TmpRays;
    gObj<Texture2D> TmpRayHeadBuffer;
    gObj<Buffer>TmpRayNextBuffer;

    gObj<Buffer> Hits;
    gObj<Buffer> Malloc;

    gObj<Texture2D> Complexity;

    gObj<InitializeRaysPipeline> initialRaysPipeline;
    gObj<InitializeRayDirectionsPipeline> initialRayDirectionsPipeline;
    gObj<BounceRaysPipeline> bounceRaysPipeline;

	gObj<ShowComplexityPipeline> showComplexity;


    gObj<Buffer> depthBuffer;

    RaymarchRT(D description) {
        this->constructingDescription = description;
    }

protected:
    void SetScene(gObj<CA4G::Scene> scene) {
        IHasScene::SetScene(scene);
        if (sceneLoader != nullptr) {
            sceneLoader->SetScene(scene);
        }
    }

    void Startup() {

		// Load pipeline to draw complexity
		_ gLoad Pipeline(showComplexity);

        // Load scene in retained mode
        if (sceneLoader == nullptr) {
            sceneLoader = new RetainedSceneLoader();
            sceneLoader->SetScene(Scene);
            _ gLoad Subprocess(sceneLoader);
        }

        constructing = new C(constructingDescription);
        constructing->sceneLoader = sceneLoader;
        _ gLoad Subprocess(constructing);

        _ gLoad Pipeline(initialRaysPipeline);
        _ gLoad Pipeline(initialRayDirectionsPipeline);
        _ gLoad Pipeline(bounceRaysPipeline);

        int maxSupportedRaysPerBounce = render_target->Width * render_target->Height * 4;

        depthBuffer = _ gCreate DepthBuffer(render_target->Width, render_target->Height);
        Complexity = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);
        Rays = _ gCreate RWStructuredBuffer<RayInfo>(maxSupportedRaysPerBounce);
        TmpRays = _ gCreate RWStructuredBuffer<RayInfo>(maxSupportedRaysPerBounce);

        RayNextBuffer = _ gCreate RWStructuredBuffer<int>(maxSupportedRaysPerBounce);
        TmpRayNextBuffer = _ gCreate RWStructuredBuffer<int>(maxSupportedRaysPerBounce);

        RayHeadBuffer = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);
        TmpRayHeadBuffer = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);

        Malloc = _ gCreate RWStructuredBuffer<int>(1);
        Hits = _ gCreate RWStructuredBuffer<RayIntersection>(maxSupportedRaysPerBounce);

        initialRaysPipeline->cameraCB = _ gCreate ConstantBuffer<Globals>();
        initialRaysPipeline->vertices = constructing->Vertices;
        initialRaysPipeline->initialTriangleIndices = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);
        initialRaysPipeline->initialCoordinates = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
        initialRaysPipeline->depthBuffer = depthBuffer;

        initialRayDirectionsPipeline->inverseCamera = _ gCreate ConstantBuffer<Globals>();
        initialRayDirectionsPipeline->screenInfo = _ gCreate ConstantBuffer<ScreenInfo>();
        initialRayDirectionsPipeline->initialTriangleIndices = initialRaysPipeline->initialTriangleIndices;
        initialRayDirectionsPipeline->initialCoordinates = initialRaysPipeline->initialCoordinates;

        bounceRaysPipeline->vertices = constructing->Vertices;
        bounceRaysPipeline->lighting = _ gCreate ConstantBuffer<Lighting>();
        bounceRaysPipeline->inverseViewing = _ gCreate ConstantBuffer<Globals>();
        bounceRaysPipeline->objectBuffer = sceneLoader->ObjectBuffer;
        bounceRaysPipeline->materialIndexBuffer = sceneLoader->MaterialIndexBuffer;
        bounceRaysPipeline->materials = sceneLoader->MaterialBuffer;
        bounceRaysPipeline->textures = sceneLoader->Textures;
        bounceRaysPipeline->textureCount = sceneLoader->TextureCount;

        perform(TestData);
    }

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

    void Graphics(gObj<GraphicsManager> manager) {
        float4x4 view, projection;
        Camera->GetMatrices(render_target->Width, render_target->Height, view, projection);

#ifndef WORLD_SPACE_RAYS
        constructing->ViewMatrix = view;
#endif

		static bool firstTime = true;
		if (firstTime) {
			ExecuteFrame(constructing);
#ifndef CONSTRUCT_ADS_EVERYFRAME
			firstTime = false;
#endif
		}

        manager gClear RT(render_target, float4(Backcolor, 1));

        manager gClear Depth(depthBuffer);

        manager gCopy ValueData(initialRaysPipeline->cameraCB, Globals{ projection, view });
        manager gClear RT(initialRaysPipeline->initialCoordinates, float4(0));
        manager gClear UAV(initialRaysPipeline->initialTriangleIndices, (unsigned int)-1);
        manager gSet Viewport(render_target->Width, render_target->Height);
        manager gSet Pipeline(initialRaysPipeline);
        manager gDispatch Triangles(sceneLoader->VertexBuffer->ElementCount);

        manager gCopy ValueData(initialRayDirectionsPipeline->inverseCamera, Globals{ projection.getInverse(), view.getInverse() });
        manager gCopy ValueData(initialRayDirectionsPipeline->screenInfo, ScreenInfo{ render_target->Width, render_target->Height });
        initialRayDirectionsPipeline->rayInfo = Rays;
        initialRayDirectionsPipeline->headBuffer = RayHeadBuffer;
        initialRayDirectionsPipeline->nextBuffer = RayNextBuffer;
        initialRayDirectionsPipeline->rayIntersections = Hits;
        manager gSet Viewport(render_target->Width, render_target->Height);
        manager gSet Pipeline(initialRayDirectionsPipeline);
        manager gSet VertexBuffer(screenVertices);
        manager gDispatch Triangles(6);

        manager gClear UAV(Complexity, 0U);
        constructing->CountHits = this->CountHits;
        constructing->CountSteps = this->CountSteps;

        int N = BOUNCES;
        bounceRaysPipeline->accumulations = render_target;

#ifdef WORLD_SPACE_RAYS
        float3 position = Light->Position;
#else
        float3 position = xyz(mul(float4(Light->Position, 1), view));
#endif
        float3 intensity = Light->Intensity;
        manager gCopy ValueData(bounceRaysPipeline->lighting, Lighting{ position, 0, intensity, 0 });
        manager gCopy ValueData(bounceRaysPipeline->inverseViewing, Globals{ projection, view.getInverse() });
        manager gClear RT(render_target, float4(0, 0, 0, 1));

        for (int b = 0; b < N; b++)
        {
            manager gSet Viewport(render_target->Width, render_target->Height);
            manager gSet Pipeline(bounceRaysPipeline);

            manager gClear UAV(Malloc, 0U);
            bounceRaysPipeline->input = Rays;
            bounceRaysPipeline->headBuffer = RayHeadBuffer;
            bounceRaysPipeline->nextBuffer = RayNextBuffer;

            bounceRaysPipeline->output = TmpRays;
            bounceRaysPipeline->outputHeadBuffer = TmpRayHeadBuffer;
            bounceRaysPipeline->outputNextBuffer = TmpRayNextBuffer;
            manager gClear UAV(TmpRayHeadBuffer, (unsigned int)-1);

            bounceRaysPipeline->malloc = Malloc;
            bounceRaysPipeline->intersections = Hits;

            manager gSet VertexBuffer(screenVertices);
            manager gDispatch Triangles(6);

            /// --- SWAP RAY BUFFERS ---
            Rays = bounceRaysPipeline->output;
            RayHeadBuffer = bounceRaysPipeline->outputHeadBuffer;
            RayNextBuffer = bounceRaysPipeline->outputNextBuffer;

            TmpRays = bounceRaysPipeline->input;
            TmpRayHeadBuffer = bounceRaysPipeline->headBuffer;
            TmpRayNextBuffer = bounceRaysPipeline->nextBuffer;
            /// ------------------------

            if (b < N - 1) // Ray cast
            {
                constructing->DetectCollision(Rays, RayHeadBuffer, RayNextBuffer, Hits, Complexity);
            }
        }

		if (CountHits || CountSteps)
		{
			showComplexity->RenderTarget = render_target;
			showComplexity->complexity = Complexity;
			manager gClear RT(render_target, Backcolor);
			manager gSet Pipeline(showComplexity);
			manager gSet VertexBuffer(screenVertices);
			manager gDispatch Triangles(6);
		}
    }

    void Frame() {
        perform(Graphics);
    }
};