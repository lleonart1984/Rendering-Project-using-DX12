#pragma once

// Drawing a single Quad with different subresources of a loaded texture

#include "..\..\CA4G\ca4gScene.h"
#include "..\Common\ConstantBuffers.h"

class Tutorial6 : public Technique, public IHasBackcolor, public IHasCamera {

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
			_ gSet VertexShader(LoadByteCode(".\\Techniques\\Tutorials\\Shaders\\PassPositionAndCoordinates_VS.cso"));
			// Loads a compiled shader object and set as pixel shader
			_ gSet PixelShader(LoadByteCode(".\\Techniques\\Tutorials\\Shaders\\BasicSampling2D_PS.cso"));
			_ gSet InputLayout({
					VertexElement { VertexElementType_Float, 3, "POSITION"},	// float3 P : POSITION
					VertexElement { VertexElementType_Float, 2, "TEXCOORD"}		// float2 C : TEXCOORD
				});
		}
		void Globals() {
			// Bindings when this pipeline state object is set.
			RTV(0, renderTarget);

			Static_SMP(0, Sampler::Point(), ShaderType_Pixel);

			CBV(0, cameraCB, ShaderType_Vertex);
		}

		void Locals() {
			SRV(0, Texture, ShaderType_Pixel);
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

	// Buffers for each transform of the subresource quad object
	gObj<Buffer>* transforms;

	// View for full loaded resource
	gObj<Texture2D> Texture;
	// Individual views for each subresource
	gObj<Texture2D> *Subresources;
	// Number of subresources
	int subresourcesCount;

protected:
	void Startup() {
		// Creates the pipeline and close it to be used by the rendering process
		_ gLoad Pipeline(pipeline);

		// Create a simple vertex buffer for the quad
		gBind(vertices) _ gCreate VertexBuffer<VERTEX>(4);
		// Create a simple index buffer for the quad
		gBind(indices) _ gCreate IndexBuffer<unsigned int>(6);

		gBind(cameraCB) _ gCreate ConstantBuffer<Globals>();

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

		// Load a texture from a file (using DirectXTex project internally)
		manager gLoad FromFile(Texture, "c:\\Users\\lleonart\\Desktop\\Models\\skybox.dds");
		//gBind(Texture) _ gCreate From(manager, "c:\\Users\\lleonart\\Desktop\\Models\\seafloor.dds");

		// Compute number of subresources
		subresourcesCount = Texture->getMipsCount()*Texture->getSlicesCount();
		// Array of subresources views
		Subresources = new gObj<Texture2D>[subresourcesCount];
		transforms = new gObj<Buffer>[subresourcesCount];

		int index = 0;
		for (int j = 0; j < Texture->getSlicesCount(); j++)
			for (int i = 0; i < Texture->getMipsCount(); i++)
				// Creates a subresource view for each posible array slice and mip slice.
			{
				Subresources[index] = Texture->CreateSubresource(j, i);
				// Creates a transform constant buffer for each quad will represent a subresource
				gBind(transforms[index]) _ gCreate ConstantBuffer<float4x4>();
				manager gCopy ValueData(transforms[index], Translate(i - Texture->getMipsCount() / 2.0f, j - Texture->getSlicesCount()/2.0f, 0)); // some offset in x axis
				
				index++;
			}
	}

	void Frame() {
		// Every frame perform a single process using a Graphics Engine
		perform(GraphicProcess);
	}

	// Graphic Process to clear the render target with a backcolor
	void GraphicProcess(GraphicsManager *manager) {

		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		// Updates camera buffer each frame
		manager gCopy ValueData(cameraCB, Globals{ proj, view });

		// Setting up per frame bindings
		pipeline->renderTarget = render_target; // this is necessary every frame because 3 different render targets can be used for triple-buffering support.
		pipeline->cameraCB = cameraCB;

		manager gClear RT(render_target, Backcolor);

		// Set the pipeline state object
		manager gSet Pipeline(pipeline);
		// Set the viewport to the dimensions of the Backbuffer
		manager gSet Viewport(BackBufferWidth, BackBufferHeight);
		// Set the vertex buffer object to the pipeline
		manager gSet VertexBuffer(vertices);
		// Set the index buffer to the pipeline
		manager gSet IndexBuffer(indices);

		for (int i = 0; i < subresourcesCount; i++)
		{
			// Locals bindings
			pipeline->transformCB = transforms[i];
			pipeline->Texture = Subresources[i];

			// Draw a quad with 4 vertices and 6 indices
			manager gDraw IndexedTriangles(6);
		}
	}
};