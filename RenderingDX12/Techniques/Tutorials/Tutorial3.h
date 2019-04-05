#pragma once

// Drawing a single triangle

class Tutorial3 : public Technique, public IHasBackcolor {

	// Speciallizes a graphics pipeline bindings object to define
	// the desired pipeline setting
	struct BasicPipeline : public GraphicsPipelineBindings {
		// Render target this pipeline will use
		gObj<Texture2D> renderTarget;
		// Setup method is overriden to load shader codes and set other default settings
		void Setup() {
			// Loads a compiled shader object and set as vertex shader
			_ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\NoTransforms_VS.cso"));
			// Setting the input layout of the pipeline
			_ gSet InputLayout({
				VertexElement { VertexElementType_Float, 3, "POSITION"},	// float3 P : POSITION
				VertexElement { VertexElementType_Float, 3, "COLOR"}		// float3 C : COLOR
				});
			// Loads a compiled shader object and set as pixel shader
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\SimpleColor_PS.cso"));
		}
		void Globals() {
			// Only one bind when this pipeline state object is set.
			RTV(0, renderTarget);
		}
	};

	// Vertex definition for triangle
	struct VERTEX {
		float3 Position;
		float3 Color;
	};

	// Object for pipeline bindings 
	gObj<BasicPipeline> pipeline;
	// Buffer object created for storing vertexes
	gObj<Buffer> vertices;

protected:
	void Startup() {

		// Creates the pipeline and close it to be used by the rendering process
		_ gLoad Pipeline(pipeline);

		// Create a simple vertex buffer for the triangle
		gBind(vertices) _ gCreate VertexBuffer<VERTEX>(3);

		// Performs a copying commanding execution for uploading data to resources
		perform(UploadData);
	}

	// A copy engine can be used to populate buffers using GPU commands.
	void UploadData(gObj<CopyingManager> manager) {
		// Copies a buffer written using an initializer_list
		manager	gCopy ListData(vertices, {
				VERTEX { float3(0.5, 0, 0), float3(1, 0, 0)},
				VERTEX { float3(-0.5, 0, 0), float3(0, 1, 0)},
				VERTEX { float3(0, 0.5, 0), float3(0, 0, 1)},
			});
	}

	void Frame() {
		// Every frame perform a single process using a Graphics Engine
		perform(GraphicProcess);
	}

	// Graphic Process to clear the render target with a backcolor
	void GraphicProcess(gObj<GraphicsManager> manager) {

		// Setting up per frame bindings
		gBind(pipeline->renderTarget) render_target; // this is necessary every frame because 3 different render targets can be used for triple-buffering support.

		// each module clear, set or draw can be used with a fluent interface to perform several related commands fluently.
		// clear expands to clearing->
		// set expands to setter->
		// draw expands to drawer->
		// and then just expands to ->, so, every command returns the same object and can be commanded again with then
		manager gClear RT(render_target, Backcolor);
		// Set the pipeline state object
		manager gSet Pipeline(pipeline);
		// Set the viewport to the dimensions of the Backbuffer
		manager gSet Viewport(BackBufferWidth, BackBufferHeight);
		// Set the vertex buffer object to the pipeline
		manager gSet VertexBuffer(vertices);
		// Draw a triangle with 3 vertices
		manager gDispatch Triangles(3);
	}
};