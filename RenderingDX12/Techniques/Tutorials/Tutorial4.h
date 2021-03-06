#pragma once

// Creating a constant buffer to control camera and per-primitive info
// This sample draws several triangles changing local transforms randomly

class Tutorial4 : public Technique, public IHasBackcolor, public IHasCamera, public IHasTriangleNumberParameter {

	// Speciallizes a graphics pipeline bindings object to define
	// the desired pipeline setting
	struct BasicPipeline : public GraphicsPipelineBindings {
		// Render target this pipeline will use
		gObj<Texture2D> renderTarget;
		// Buffer will be bound as global cb
		gObj<Buffer> cameraCB;
		// Buffer will be bound as local cb for world transform
		gObj<Buffer> currentTransformCB;

		// Setup method is overriden to load shader codes and set other default settings
		void Setup() {
			// Loads a compiled shader object and set as vertex shader
			_ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\SimpleTransform_VS.cso"));
			// Loads a compiled shader object and set as pixel shader
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\SimpleColor_PS.cso"));
			// Sets the input layout for this pipeline
			_ gSet InputLayout({
					VertexElement { VertexElementType_Float, 3, "POSITION"},	// float3 P : POSITION
					VertexElement { VertexElementType_Float, 3, "COLOR"}		// float3 C : COLOR
				});
		}
		void Globals() {
			// Only one bind when this pipeline state object is set.
			RTV(0, renderTarget);

			// Binds the constant buffer to the vertex stage only
			CBV(0, cameraCB, ShaderType_Vertex);
		}
		// This method represents the resources will be bound every draw call.
		// Each different triangle must change the currentTransformCB
		void Locals() {
			// Binds the constant buffer to the vertex stage only
			CBV(1, currentTransformCB, ShaderType_Vertex);
		}
	};

	// Vertex definition for triangle
	struct VERTEX {
		float3 Position;
		float3 Color;
	};

	// Defined struct to map on CPU the Constant Buffer for camera
	struct Globals {
		float4x4 projection;
		float4x4 view;
	};

	gObj<BasicPipeline> pipeline;
	// Buffer object created for storing vertexes
	gObj<Buffer> vertices;
	// Constant buffer created for camera
	gObj<Buffer> cameraCB;
	// Constant buffers created for world tranform of each triangle
	gObj<Buffer>* transformsCBs;


public:
	Tutorial4() {
	}

	~Tutorial4() {
		if (transformsCBs)
			delete[] transformsCBs;
	}

protected:
	void Startup() {
		// Creates the pipeline and close it to be used by the rendering process
		_ gLoad Pipeline(pipeline);

		// Create a simple vertex buffer for the triangle
		gBind(vertices) _ gCreate VertexBuffer<VERTEX>(3);
		// Create constant buffer float global transforms (view and projection)
		gBind(cameraCB) _ gCreate ConstantBuffer<Globals>();
		// Create an array of constant buffers for local transforms
		transformsCBs = new gObj<Buffer>[MAX_NUMBER_OF_TRIANGLES];
		for (int i = 0; i < MAX_NUMBER_OF_TRIANGLES; i++)
			gBind(transformsCBs[i]) _ gCreate ConstantBuffer<Globals>();
		// Performs a copying commanding execution for uploading data to resources
		perform(UploadData);
	}

	// A copy engine can be used to populate buffers using GPU commands.
	void UploadData(gObj<CopyingManager> manager) {
		// Copies a buffer written using an initializer_list
		manager	gCopy ListData(vertices, {
				VERTEX { float3(-0.5, 0, 0), float3(1, 0, 0)},
				VERTEX { float3(0.5, 0, 0), float3(0, 1, 0)},
				VERTEX { float3(0, 0.5, 0), float3(0, 0, 1)},
			});
	}

	void Frame() {
		// Every frame perform a single process using a Graphics Engine
		perform(DrawTriangles);
	}

	double time;

	// Graphic Process render the triangles
	void DrawTriangles(gObj<GraphicsManager> manager) {

		// each module clear, set or draw can be used with a fluent interface to perform several related commands fluently.
		// clear expands to clearing->
		// set expands to setter->
		// draw expands to drawer->
		// and then just expands to ->, so, every command returns the same object and can be commanded again with then
		manager gClear RT(render_target, Backcolor);

		// Get matrices from Camera object
		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
		// Update camera constant buffer
		manager gCopy ValueData(cameraCB, Globals{ proj, view });

		// Compute time for animation
		time = GetTickCount() % 100000 / 1000.0;

		// Setting up per frame bindings
		pipeline->renderTarget = render_target; // this is necessary every frame because 3 different render targets can be used for triple-buffering support.
		pipeline->cameraCB = cameraCB;

		// Set the pipeline state object
		manager	gSet Pipeline(pipeline);
		// Set the viewport to the dimensions of the Backbuffer
		manager gSet Viewport(render_target->Width, render_target->Height);
		// Set the vertex buffer object to the pipeline
		manager gSet VertexBuffer(vertices);

		// Update all transforms for each triangle
		for (int i = 0; i < NumberOfTriangles; i++)
		{
			float x = sin(time * 4 + (i ^ (i + 13)) % 10000);
			float y = cos(time * 2 + (i + i ^ (i + 15)) % 10000);
			float z = sin(time * 5 + (i - i ^ (i + 21)) % 10000);
			manager gCopy ValueData(transformsCBs[i], mul(Translate(x, y, z), Rotate(time, normalize(float3(z, x, y)))));

			// setup pipeline local binds
			pipeline->currentTransformCB = transformsCBs[i];
			// Draw a triangle with 3 vertices
			manager gDispatch Triangles(3);
		}
	}
};
