#pragma once

// Rendering to a texture and drawing a single Quad with it.

#include "..\..\CA4G\ca4gScene.h"
#include "..\Common\ConstantBuffers.h"

class Tutorial7 : public Technique, public IHasBackcolor, public IHasCamera {

	// Speciallizes a graphics pipeline bindings object to define
	// the desired pipeline setting
	struct BasicPipeline : public GraphicsPipelineBindings {
		// Render target this pipeline will use
		gObj<Texture2D> renderTarget;
		gObj<Texture2D> Texture;
		gObj<Buffer> cameraCB;
		gObj<Buffer> transformCB;

		// Setup method is overriden to load shader codes and set other default settings
		void Setup() {
			// Loads a compiled shader object and set as vertex shader
			_ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\PassPositionAndCoordinates_VS.cso"));
			// Loads a compiled shader object and set as pixel shader
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\BasicSampling2D_PS.cso"));
			_ gSet InputLayout({
					VertexElement { VertexElementType_Float, 3, "POSITION"},	// float3 P : POSITION
					VertexElement { VertexElementType_Float, 2, "TEXCOORD"}		// float2 C : TEXCOORD
				});
		}
		void Globals() {
			// Bindings when this pipeline state object is set.
			RTV(0, renderTarget);

			Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);
			SRV(0, Texture, ShaderType_Pixel);

			CBV(0, cameraCB, ShaderType_Vertex);
			CBV(1, transformCB, ShaderType_Vertex);
		}
	};

	// Vertex definition for the quad
	struct VERTEX {
		float3 Position;
		float2 Coordinates;
	};

	gObj<BasicPipeline> pipeline;

	// Buffer object created for storing vertexes
	gObj<Buffer> vertices;
	// Buffer object created for storing indices
	gObj<Buffer> indices;

	// Buffer for global transforms
	gObj<Buffer> cameraCB;

	// Buffer for transform of the quad object
	gObj<Buffer> transform;

	// Texture to be used as render target and shader resource view
	gObj<Texture2D> Texture[2];

protected:
	void Startup() {
		// Creates the pipeline and close it to be used by the rendering process
		_ gLoad Pipeline(pipeline);

		// Create a simple vertex buffer for the quad
		gBind(vertices) _ gCreate VertexBuffer<VERTEX>(4);
		// Create a simple index buffer for the quad
		gBind(indices) _ gCreate IndexBuffer<unsigned int>(6);

		gBind(cameraCB) _ gCreate ConstantBuffer<Globals>();
		gBind(transform) _ gCreate ConstantBuffer<float4x4>();

		for (int i=0; i<2; i++)
			gBind(Texture[i]) _ gCreate DrawableTexture2D<RGBA>(512, 512);

		// Performs a copying commanding execution for uploading data to resources
		perform(UploadData);
	}

	// A copy engine can be used to populate buffers using GPU commands.
	void UploadData(CopyingManager *manager) {
			// Copies a buffer written using an initializer_list
		manager gCopy ListData(vertices, {
				VERTEX { float3(-0.5, -0.5, 0), float2(0, 1)},
				VERTEX { float3(0.5, -0.5, 0), float2(1, 1)},
				VERTEX { float3(0.5, 0.5, 0), float2(1, 0)},
				VERTEX { float3(-0.5, 0.5, 0), float2(0, 0)},
			});
		manager gCopy ListData(indices, {
							0, 1, 2,
							0, 2, 3
					});
	}

	void Frame() {
		// Every frame perform a single process using a Graphics Engine
		perform(GraphicProcess);
	}

	void DrawSceneOn(gObj<Texture2D> renderTarget, gObj<Texture2D> texture, float3 backColor, GraphicsManager *manager) {
		pipeline->renderTarget = renderTarget;
		pipeline->Texture = texture;

		manager gClear RT(pipeline->renderTarget, backColor);

		// Set the pipeline state object
		manager gSet Pipeline(pipeline);
		// Set the viewport to the dimensions of the Backbuffer
		manager gSet Viewport(renderTarget->Width, renderTarget->Height);
		// Set the vertex buffer object to the pipeline
		manager gSet VertexBuffer(vertices);
		// Set the index buffer to the pipeline
		manager gSet IndexBuffer(indices);

		// Draw a quad with 4 vertices and 6 indices
		manager gDispatch IndexedTriangles(6);
	}

	// Graphic Process to clear the render target with a backcolor
	void GraphicProcess(GraphicsManager *manager) {

		static int frameIndex = 0;
		
		frameIndex++;

		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		// Updates camera buffer each frame and transform
		manager gCopy ValueData(cameraCB, Globals{ proj, view });
		manager gCopy ValueData(transform, float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));

		// Setting up per frame bindings
		pipeline->cameraCB = cameraCB;
		pipeline->transformCB = transform;

		DrawSceneOn(Texture[0], Texture[1], float3(1, 1, 1) - Backcolor, manager);
		DrawSceneOn(Texture[1], Texture[0], Backcolor, manager);

		DrawSceneOn(render_target, Texture[0], Backcolor, manager);
	}
};