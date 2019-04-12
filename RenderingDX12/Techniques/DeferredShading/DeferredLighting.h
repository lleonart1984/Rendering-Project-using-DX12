#pragma once

#include "GBufferConstruction.h"

class DeferredShadingTechnique : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

	gObj<GBufferConstruction> gBuffer;

	struct DeferredLightingPipeline : ShowTexturePipeline {
		gObj<Buffer> Lighting;
		gObj<Texture2D> positions;
		gObj<Texture2D> normals;
		gObj<Texture2D> coordinates;
		gObj<Texture2D> materialIndices;
		gObj<Buffer> materials;

		gObj<Texture2D>* Textures = nullptr;
		int TextureCount;

		void Setup() {
			ShowTexturePipeline::Setup();
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\DeferredShading\\DeferredLighting_PS.cso"));
		}

		void Globals() {
			RTV(0, RenderTarget);
			CBV(0, Lighting, ShaderType_Pixel);
			SRV(0, positions, ShaderType_Pixel);
			SRV(1, normals, ShaderType_Pixel);
			SRV(2, coordinates, ShaderType_Pixel);
			SRV(3, materialIndices, ShaderType_Pixel);
			SRV(4, materials, ShaderType_Pixel);
			SRV_Array(5, Textures, TextureCount, ShaderType_Pixel);

			Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);
		}
	};

	gObj<Buffer> screenVertices;
	gObj<DeferredLightingPipeline> pipeline;

	void Startup() {
		// Load and setup gbuffer construction process
		gBuffer = new GBufferConstruction(render_target->Width, render_target->Height);
		gBuffer->Scene = this->Scene;
		_ gLoad Subprocess(gBuffer);

		// Load the pipeline for drawing deferred lighting
		_ gLoad Pipeline(pipeline);

		pipeline->positions = gBuffer->pipeline->GBuffer_P;
		pipeline->normals = gBuffer->pipeline->GBuffer_N;
		pipeline->coordinates = gBuffer->pipeline->GBuffer_C;
		pipeline->materialIndices = gBuffer->pipeline->GBuffer_M;
		pipeline->materials = gBuffer->pipeline->materials;
		pipeline->Textures = gBuffer->pipeline->Textures;
		pipeline->TextureCount = gBuffer->pipeline->TextureCount;

		pipeline->Lighting = _ gCreate ConstantBuffer<Lighting>();

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);
	}

	void CreatingAssets(gObj<CopyingManager> manager) {

		// load screen vertices
		screenVertices = _ gCreate VertexBuffer<float2>(6);
		manager gCopy ListData(screenVertices, {
				float2(-1, 1),
				float2(1, 1),
				float2(1, -1),

				float2(1, -1),
				float2(-1, -1),
				float2(-1, 1),
			});
	}

	void Frame() {
		// Construct GBuffer
		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		gBuffer->ViewMatrix = view;
		gBuffer->ProjectionMatrix = proj;
		ExecuteFrame(gBuffer);

		perform(DrawDeferredShading);
	}

	void DrawDeferredShading(gObj<GraphicsManager> manager) {
		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		manager gCopy ValueData(pipeline->Lighting, Lighting{
				mul(float4(Light->Position, 1), view).getXYZHomogenized(), 0,
				Light->Intensity
			});

		manager gClear RT(render_target, float4(1, 1, 0, 1));
		pipeline->RenderTarget = render_target;
		manager gSet Pipeline(pipeline);
		manager gSet Viewport(render_target->Width, render_target->Height);
		manager gSet VertexBuffer(screenVertices);
		manager gDispatch Triangles(6);
	}
};