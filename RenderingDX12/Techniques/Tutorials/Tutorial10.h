#pragma once

// Drawing a single triangle with ray-tracing

struct DXRBasic : public RTPipelineManager {

	gObj<RayGenerationHandle> MyRaygenShader;
	gObj<ClosestHitHandle> MyClosestHitShader;
	gObj<MissHandle> MyMissShader;

	class DX_RTX_Sample_RT : public DXIL_Library<DXRBasic> {
		void Setup() {
			_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\DX_RTX_Basic_RT.cso"));

			_ gLoad Shader(Context()->MyRaygenShader, L"MyRaygenShader");
			_ gLoad Shader(Context()->MyClosestHitShader, L"MyClosestHitShader");
			_ gLoad Shader(Context()->MyMissShader, L"MyMissShader");
		}
	};
	gObj<DX_RTX_Sample_RT> _Library;

	class MyProgram : public RTProgram<DXRBasic>
	{
	protected:
		void Setup() {
			_ gSet Payload(16);
			_ gLoad Shader(Context()->MyRaygenShader);
			_ gLoad Shader(Context()->MyMissShader);
			_ gCreate HitGroup(ClosestHit, Context()->MyClosestHitShader, {}, {});
		}

		void Globals() override {
			UAV(0, Output);
			ADS(0, Scene);
		}

		void RayGeneration_Locals() override {
			CBV(0, RayGenConstantBuffer);
		}
	public:
		gObj<SceneOnGPU> Scene;

		struct Viewport { float l, t, r, b; };
		struct RayGenConstantBuffer {
			Viewport viewport;
			Viewport stencil;
		} RayGenConstantBuffer;

		gObj<Texture2D> Output;

		gObj<HitGroupHandle> ClosestHit;
	};
	gObj<MyProgram> _Program;

	void Setup() override {
		_ gLoad Library(_Library);
		_ gLoad Program(_Program);
	}
};

class Tutorial10 : public Technique {
	// Vertex definition for triangle
	struct VERTEX {
		float3 Position;
		float3 Color;

		static auto & Layout() {
			static VertexElement result[] =
			{
				VertexElement(VertexElementType_Float, 3, "POSITION"),
				VertexElement(VertexElementType_Float, 3, "COLOR")
			};
			return result;
		}
	};

	// Object for rt pipeline bindings 
	gObj<DXRBasic> pipeline;

	// Buffer object created for storing vertexes
	gObj<Buffer> vertices;

	gObj<Texture2D> rtRenderTarget;

	gObj<Buffer> Scene;

	void Startup() {
		// Creates the pipeline and close it to be used by the rendering process
		_ gLoad Pipeline(pipeline);

		// _ gCreate ConstantBuffer<RayGenConstantBuffer>();

		rtRenderTarget = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);

		// Create a simple vertex buffer for the triangle
		//vertices = _ gCreate VertexBuffer<VERTEX>(3);
		vertices = _ gCreate GenericBuffer<VERTEX>(D3D12_RESOURCE_STATE_GENERIC_READ, 3, CPU_WRITE_GPU_READ);

		// Performs a copying commanding execution for uploading data to resources
		perform(UploadData);

		// Performs a building process to generate acceleration data structures (scene)
		// using a DXRManager object.
		perform(BuildScene);
	}

	void BuildScene(gObj<DXRManager> manager) {
		auto geometries = manager gCreate TriangleGeometries();
		geometries gSet VertexBuffer(vertices, VERTEX::Layout());
		geometries gLoad Geometry(0, 3);
		gObj<GeometriesOnGPU> geometriesOnGPU;
		geometriesOnGPU = geometries gCreate BakedGeometry();
		auto instances = manager gCreate Instances();
		instances gLoad Instance(geometriesOnGPU);
		this->Scene = instances gCreate BakedScene();
	}

	// A copy engine can be used to populate buffers using GPU commands.
	void UploadData(gObj<CopyingManager> manager) {
		// Copies a buffer written using an initializer_list
		manager	gCopy ListData(vertices, {
				VERTEX { float3(0.8, -0.4, 0), float3(1, 0, 0)},
				VERTEX { float3(-0.8, -0.4, 0), float3(0, 1, 0)},
				VERTEX { float3(0, 0.8, 0), float3(0, 0, 1)},
			});
	}

	void Frame() {
		perform(Raytracing);
	}

	void Raytracing(gObj<DXRManager> manager) {
		auto rtProgram = pipeline->_Program;
		rtProgram->Output = rtRenderTarget;
		rtProgram->Scene = this->Scene;
		pipeline->_Program->RayGenConstantBuffer = {
			{ -0.9, -0.9, 0.9, 0.9},
			{-0.9, -0.9, 0.9, 0.9}
		};
		manager gSet Pipeline(pipeline);
		manager gSet Program(rtProgram);
		manager gSet Miss(pipeline->MyMissShader, 0);
		manager gSet HitGroup(rtProgram->ClosestHit, 0);
		manager gSet RayGeneration(pipeline->MyRaygenShader);
		manager gDispatch Rays(render_target->Width, render_target->Height);
		manager gCopy All(render_target, rtRenderTarget);
	}
};