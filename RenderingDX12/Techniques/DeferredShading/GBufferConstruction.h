#pragma once
#include "..\Common\RetainedSceneLoader.h"
#include "..\Common\ConstantBuffers.h"
#include "ca4G.h"

using namespace CA4G;

class GBufferConstruction : public Technique, public IHasScene {
public:

	gObj<RetainedSceneLoader> sceneLoader;

	class GBufferPipeline : public GraphicsPipelineBindings {
	public:
		// RTs
		gObj<Texture2D> GBuffer_P;
		gObj<Texture2D> GBuffer_N;
		gObj<Texture2D> GBuffer_C;
		gObj<Texture2D> GBuffer_M;
		// DBVs
		gObj<Texture2D> depthBuffer;

		// CBs
		gObj<Buffer> globals;

		// SRVs
		gObj<Buffer> vertices;
		gObj<Buffer> objectIds;

		gObj<Buffer> transforms;
		gObj<Buffer> materialIndices;

		gObj<Buffer> materials;

		gObj<Texture2D>* Textures = nullptr;
		int TextureCount;

	protected:
		void Setup() {
			_ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\DeferredShading\\DeferredShading_VS.cso"));
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\DeferredShading\\DeferredShading_PS.cso"));
			_ gSet RenderTargetFormatAt(0, DXGI_FORMAT_R32G32B32A32_FLOAT);
			_ gSet RenderTargetFormatAt(1, DXGI_FORMAT_R32G32B32A32_FLOAT);
			_ gSet RenderTargetFormatAt(2, DXGI_FORMAT_R32G32B32A32_FLOAT);
			_ gSet RenderTargetFormatAt(3, DXGI_FORMAT_R32_SINT);
			_ gSet InputLayout({});
			_ gSet DepthTest();
		}

		void Globals()
		{
			RTV(0, GBuffer_P);
			RTV(1, GBuffer_N);
			RTV(2, GBuffer_C);
			RTV(3, GBuffer_M);

			DBV(depthBuffer);

			Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);

			SRV(0, materialIndices, ShaderType_Pixel);
			SRV(1, materials, ShaderType_Pixel);
			CBV(0, globals, ShaderType_Vertex);
			SRV(0, vertices, ShaderType_Vertex);
			SRV(1, objectIds, ShaderType_Vertex);
			SRV(2, transforms, ShaderType_Vertex);
			SRV_Array(2, Textures, TextureCount, ShaderType_Pixel);
		}
	};
	gObj<GBufferPipeline> pipeline;

	const int width, height;

	GBufferConstruction(int width, int height) : width(width), height(height) {
	}
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;

protected:
	void SetScene(gObj<CA4G::Scene> scene)
	{
		IHasScene::SetScene(scene);
		if (sceneLoader != nullptr)
			sceneLoader->SetScene(scene);
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

		// Create depth buffer resource
		pipeline->depthBuffer = _ gCreate DepthBuffer(width, height);

		// Create globals VS constant buffer
		pipeline->globals = _ gCreate ConstantBuffer<Globals>();

		pipeline->GBuffer_P = _ gCreate DrawableTexture2D<float4>(width, height);
		pipeline->GBuffer_N = _ gCreate DrawableTexture2D<float4>(width, height);
		pipeline->GBuffer_C = _ gCreate DrawableTexture2D<float4>(width, height);
		pipeline->GBuffer_M = _ gCreate DrawableTexture2D<int>(width, height);

		pipeline->TextureCount = sceneLoader->TextureCount;
		pipeline->Textures = sceneLoader->Textures;
		pipeline->vertices = sceneLoader->VertexBuffer;
		pipeline->objectIds = sceneLoader->ObjectBuffer;
		pipeline->materialIndices = sceneLoader->MaterialIndexBuffer;
		pipeline->materials = sceneLoader->MaterialBuffer;
		pipeline->transforms = sceneLoader->TransformBuffer;
	}

	
	void Graphics(gObj<GraphicsManager> manager) {
		manager gCopy ValueData(pipeline->globals, Globals{ ProjectionMatrix, ViewMatrix });

		manager gClear RT(pipeline->GBuffer_P, float4(0, 0, 0, 0));
		manager gClear Depth(pipeline->depthBuffer, 1);

		manager gSet Viewport(width, height);
		manager gSet Pipeline(pipeline);

		manager gDispatch Triangles(sceneLoader->VertexBuffer->ElementCount);
	}

	void Frame() {
		perform(Graphics);
	}
};