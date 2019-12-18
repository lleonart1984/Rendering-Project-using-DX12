#pragma once

#include "../../../stdafx.h"
#include "RaymarchRT_Constants.h"

#define BOUNCES 2

class GenerateInitialRaysPipeline : public ComputePipelineBindings {
public:

	gObj<Texture2D> Positions;
	gObj<Texture2D> Normals;
	gObj<Texture2D> Coordinates;
	gObj<Texture2D> MaterialIndices;
	gObj<Buffer> Materials;
	gObj<Texture2D>* Textures;
	int TextureCount;

	gObj<Buffer> ViewToWorld;

	gObj<Buffer> OutputRays;
	gObj<Texture2D> OutputHead;
	gObj<Buffer> OutputNext;
	gObj<Buffer> Malloc;

protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\RaymarchRT\\Shaders\\GenerateInitialRays_CS.cso"));
    }

    void Globals() {
        CBV(0, ViewToWorld, ShaderType_Any);

		UAV(0, OutputRays, ShaderType_Any);
		UAV(1, OutputHead, ShaderType_Any);
		UAV(2, OutputNext, ShaderType_Any);
		UAV(3, Malloc, ShaderType_Any);

		SRV(0, Positions, ShaderType_Any);
		SRV(1, Normals, ShaderType_Any);
		SRV(2, Coordinates, ShaderType_Any);
		SRV(3, MaterialIndices, ShaderType_Any);
		SRV(4, Materials, ShaderType_Any);
		SRV_Array(5, Textures, TextureCount, ShaderType_Any);

		Static_SMP(0, Sampler::Linear(), ShaderType_Any);
    }
};

class RaytracingScatteringPipeline : public ComputePipelineBindings {
public:
	gObj<Buffer> OutputRays; // u0
	gObj<Texture2D> OutputHead; // u1
	gObj<Buffer> OutputNext; // u2
	gObj<Buffer> Malloc; // u3
	gObj<Texture2D> Output; // u4
	gObj<Texture2D> Accum; // u5
    
	// CBs
	gObj<Buffer> lightTransforms; // b0
	gObj<Buffer> lighting; // b1
	gObj<Buffer> ProgressiveInfo; // b2
    // SRVs
    gObj<Buffer> input; //t0
    gObj<Texture2D> headBuffer; //t1
    gObj<Buffer> nextBuffer; //t2
    gObj<Buffer> intersections; //t3

    gObj<Buffer> vertices; // t4
    gObj<Buffer> objectBuffer; // t5
    gObj<Buffer> materialIndexBuffer; //t6
	gObj<Buffer> materials; //t7
	gObj<Texture2D> lightPositions; //t8
	gObj<Texture2D> Background; // t9

    gObj<Texture2D>* textures = nullptr; // t10
    int textureCount;



protected:
    void Setup() {
        _ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Raymarching\\RaymarchRT\\Shaders\\RaytracingScattering_CS.cso"));
    }

    void Globals() {
		CBV(0, lightTransforms, ShaderType_Any);
		CBV(1, lighting, ShaderType_Any);
		CBV(2, ProgressiveInfo, ShaderType_Any);

		UAV(0, OutputRays, ShaderType_Any);
		UAV(1, OutputHead, ShaderType_Any);
		UAV(2, OutputNext, ShaderType_Any);
		UAV(3, Malloc, ShaderType_Any);
		UAV(4, Output, ShaderType_Any);
		UAV(5, Accum, ShaderType_Any);

		SRV(0, input, ShaderType_Any);
		SRV(1, headBuffer, ShaderType_Any);
		SRV(2, nextBuffer, ShaderType_Any);
		SRV(3, intersections, ShaderType_Any);
		SRV(4, vertices, ShaderType_Any);
		SRV(5, objectBuffer, ShaderType_Any);
		SRV(6, materialIndexBuffer, ShaderType_Any);
		SRV(7, materials, ShaderType_Any);
		SRV(8, lightPositions, ShaderType_Any);
		SRV(9, Background, ShaderType_Any);
		SRV_Array(10, textures, textureCount, ShaderType_Any);
		Static_SMP(0, Sampler::Linear(), ShaderType_Any);
		Static_SMP(1, Sampler::LinearWithoutMipMaps(), ShaderType_Any);
    }
};

template<typename C, typename D>
class RaymarchRT : public DirectLightingTechnique, public IHasRaymarchDebugInfo {
public:
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

    gObj<GenerateInitialRaysPipeline> generationPipeline;
    gObj<RaytracingScatteringPipeline> scatteringPipeline;

	gObj<ShowComplexityPipeline> showComplexity;

    RaymarchRT(D description) {
        this->constructingDescription = description;
    }

protected:

    void Startup() {

		DirectLightingTechnique::Startup();

		// Load pipeline to draw complexity
		_ gLoad Pipeline(showComplexity);

        constructing = new C(constructingDescription);
        constructing->sceneLoader = sceneLoader;
        _ gLoad Subprocess(constructing);

        _ gLoad Pipeline(generationPipeline);
        _ gLoad Pipeline(scatteringPipeline);

        int maxSupportedRaysPerBounce = render_target->Width * render_target->Height * 4;

        Complexity = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);
        Rays = _ gCreate RWStructuredBuffer<RayInfo>(maxSupportedRaysPerBounce);
        TmpRays = _ gCreate RWStructuredBuffer<RayInfo>(maxSupportedRaysPerBounce);

        RayNextBuffer = _ gCreate RWStructuredBuffer<int>(maxSupportedRaysPerBounce);
        TmpRayNextBuffer = _ gCreate RWStructuredBuffer<int>(maxSupportedRaysPerBounce);

        RayHeadBuffer = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);
        TmpRayHeadBuffer = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);

        Malloc = _ gCreate RWStructuredBuffer<int>(1);
        Hits = _ gCreate RWStructuredBuffer<RayIntersection>(maxSupportedRaysPerBounce);

		// From viewer GBuffer
		generationPipeline->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		generationPipeline->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		generationPipeline->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		generationPipeline->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		generationPipeline->ViewToWorld = _ gCreate ConstantBuffer<float4x4>();
		// From scene loader
		generationPipeline->Materials = sceneLoader->MaterialBuffer;
		generationPipeline->Textures = sceneLoader->Textures;
		generationPipeline->TextureCount = sceneLoader->TextureCount;
		// For ray-generation
		generationPipeline->Malloc = Malloc;

		scatteringPipeline->Background = DirectLighting;
		scatteringPipeline->Accum = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		scatteringPipeline->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);

		scatteringPipeline->lighting = computeDirectLighting->Lighting;
		scatteringPipeline->lightTransforms = computeDirectLighting->LightTransforms;
		scatteringPipeline->lightPositions = gBufferFromLight->pipeline->GBuffer_P;
		
		scatteringPipeline->Malloc = Malloc;
		
		scatteringPipeline->ProgressiveInfo = _ gCreate ConstantBuffer<int>();
		
		scatteringPipeline->materialIndexBuffer = sceneLoader->MaterialIndexBuffer;
		scatteringPipeline->materials = sceneLoader->MaterialBuffer;
		scatteringPipeline->vertices = sceneLoader->VertexBuffer;
		scatteringPipeline->objectBuffer = sceneLoader->ObjectBuffer;
		scatteringPipeline->textures = sceneLoader->Textures;
		scatteringPipeline->textureCount = sceneLoader->TextureCount;

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

	void Graphics(gObj<GraphicsManager> gmanager) {

		auto manager = gmanager.Dynamic_Cast<ComputeManager>();

		float4x4 view, projection;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, projection);

#ifndef WORLD_SPACE_RAYS
		constructing->ViewMatrix = view;
#endif

		static bool firstTime = true;
		if (firstTime) {
			ExecuteFrame(constructing);
#ifndef REBUILT_ADS_EVERY_FRAME
			firstTime = false;
#endif
		}

		generationPipeline->OutputRays = Rays;
		generationPipeline->OutputHead = RayHeadBuffer;
		generationPipeline->OutputNext = RayNextBuffer;

		manager gClear UAV(scatteringPipeline->Output, 0u);
		manager gClear UAV(scatteringPipeline->Accum, 0u);

		manager gCopy ValueData(generationPipeline->ViewToWorld, view.getInverse());
		manager gClear UAV(RayHeadBuffer, (unsigned int)-1);
		manager gClear UAV(Malloc, 0u);
		manager gSet Pipeline(generationPipeline);
		manager gDispatch Threads(
			ceil(render_target->Width / (double)CS_2D_GROUPSIZE),
			ceil(render_target->Height / (double)CS_2D_GROUPSIZE));

		manager gClear UAV(Complexity, 0U);
		constructing->CountHits = this->CountHits;
		constructing->CountSteps = this->CountSteps;

		int N = RAY_TRACING_MAX_BOUNCES;

		for (int b = 0; b < N; b++)
		{
			constructing->DetectCollision(Rays, RayHeadBuffer, RayNextBuffer, Hits, Complexity);

			scatteringPipeline->input = Rays;
			scatteringPipeline->headBuffer = RayHeadBuffer;
			scatteringPipeline->nextBuffer = RayNextBuffer;

			scatteringPipeline->OutputRays = TmpRays;
			scatteringPipeline->OutputHead = TmpRayHeadBuffer;
			scatteringPipeline->OutputNext = TmpRayNextBuffer;
			manager gClear UAV(Malloc, 0U);
			manager gClear UAV(TmpRayHeadBuffer, (unsigned int)-1);
			scatteringPipeline->Malloc = Malloc;
			scatteringPipeline->intersections = Hits;

			manager gSet Pipeline(scatteringPipeline);
			manager gDispatch Threads(
				ceil(render_target->Width / (double)CS_2D_GROUPSIZE),
				ceil(render_target->Height / (double)CS_2D_GROUPSIZE));

			/// --- SWAP RAY BUFFERS ---
			Rays = scatteringPipeline->OutputRays;
			RayHeadBuffer = scatteringPipeline->OutputHead;
			RayNextBuffer = scatteringPipeline->OutputNext;

			TmpRays = scatteringPipeline->input;
			TmpRayHeadBuffer = scatteringPipeline->headBuffer;
			TmpRayNextBuffer = scatteringPipeline->nextBuffer;
			/// ------------------------
		}

		// Copy output texture to the render target
		manager gCopy All(render_target, scatteringPipeline->Output);
	}

	void ShowComplexity(gObj<GraphicsManager> manager) {
		showComplexity->RenderTarget = render_target;
		showComplexity->complexity = Complexity;
		manager gClear RT(render_target, float4(1, 0, 0, 1));
		manager gSet Viewport(render_target->Width, render_target->Height);
		manager gSet Pipeline(showComplexity);
		manager gSet VertexBuffer(screenVertices);
		manager gDispatch Triangles(6);
	}

    void Frame() {

		DirectLightingTechnique::Frame();

        perform(Graphics);

		if (CountHits || CountSteps)
			perform(ShowComplexity);
    }
};