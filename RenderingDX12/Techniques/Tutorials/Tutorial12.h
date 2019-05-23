#pragma once

#include "../../stdafx.h"
#define RES 128

class Tutorial12 : public Technique {

	struct DXRBasic : public RTPipelineManager {

		gObj<RayGenerationHandle> MyRaygenShader;
		gObj<ClosestHitHandle> MyClosestHitShader;
		gObj<IntersectionHandle> MyIntersectionShader;
		gObj<AnyHitHandle> MyAnyHitShader;
		gObj<MissHandle> MyMissShader;

		class DX_RTX_Sample_RT : public DXIL_Library<DXRBasic> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\DXR_ProceduralGeometrySample_RT.cso"));

				_ gLoad Shader(Context()->MyRaygenShader, L"MyRaygenShader");
				_ gLoad Shader(Context()->MyClosestHitShader, L"MyClosestHitShader");
				_ gLoad Shader(Context()->MyAnyHitShader, L"MyAnyHit");
				_ gLoad Shader(Context()->MyIntersectionShader, L"MyIntersectionShader");
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
				_ gCreate HitGroup(ClosestHit, Context()->MyClosestHitShader, Context()->MyAnyHitShader, Context()->MyIntersectionShader);
			}

			void Globals() override {
				UAV(0, Output);
				ADS(0, Scene);
			}

		public:
			gObj<SceneOnGPU> Scene;

			gObj<Texture2D> Output;

			gObj<HitGroupHandle> ClosestHit;
		};
		gObj<MyProgram> _Program;

		void Setup() override {
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};

	struct UpdateAABBsPipeline : public GraphicsPipelineBindings {
		void Setup() {
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Tutorials\\Shaders\\UpdateAABBs_PS.cso"));
			_ gSet VertexShader(ShaderLoader::DrawScreenVS());
			_ gSet InputLayout({
					VertexElement(VertexElementType_Float, 2, "POSITION")
				});
		}
		
		gObj<Buffer> Boxes;

		void Globals() {
			UAV(0, Boxes, ShaderType_Pixel);
		}
	};

	// Object for rt pipeline bindings 
	gObj<DXRBasic> pipeline;
	gObj<UpdateAABBsPipeline> updatePipeline;

	gObj<Texture2D> rtRenderTarget;
	gObj<Buffer> aabbs;
	gObj<SceneOnGPU> Scene;
	gObj<Buffer> screenVertices;

	void Startup() {
		// Creates the pipeline and close it to be used by the rendering process
		_ gLoad Pipeline(pipeline);
		_ gLoad Pipeline(updatePipeline);

		// _ gCreate ConstantBuffer<RayGenConstantBuffer>();

		rtRenderTarget = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		aabbs = _ gCreate RWAccelerationDatastructureBuffer<D3D12_RAYTRACING_AABB>(RES*RES);

		//aabbs = _ gCreate GenericBuffer<D3D12_RAYTRACING_AABB>(D3D12_RESOURCE_STATE_GENERIC_READ, RES*RES, CPU_WRITE_GPU_READ,
		//	D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		updatePipeline->Boxes = aabbs;

		screenVertices = _ gCreate VertexBuffer <float2>(6);

		// Performs a copying commanding execution for uploading data to resources
		perform(UploadData);

		flush_all_to_gpu;

		// Performs a building process to generate acceleration data structures (scene)
		// using a DXRManager object.
		perform(BuildScene);
	}

	gObj<GeometriesOnGPU> geometriesOnGPU;

	void BuildScene(gObj<DXRManager> manager) {
		auto geometries = manager gCreate ProceduralGeometries();
		geometries gSet AABBs(aabbs);
		geometries gLoad Geometry(0, RES*RES);
		geometriesOnGPU = geometries gCreate BakedGeometry(true, true);

		auto instances = manager gCreate Instances();
		instances gLoad Instance(geometriesOnGPU);
		this->Scene = instances gCreate BakedScene(true, true);
	}

	// A copy engine can be used to populate buffers using GPU commands.
	void UploadData(gObj<CopyingManager> manager) {
		D3D12_RAYTRACING_AABB* aabbsData = new D3D12_RAYTRACING_AABB[RES*RES];
		for (int j = 0; j < RES; j++)
			for (int i = 0; i < RES; i++)
			{
				float x = -0.5 + i * 1.0f / RES;// +0.5f*sinf(ImGui::GetTime()*0.1f + 3.14f*(float)j / RES);
				float y = -0.5 + j * 1.0f / RES;// +0.5f*cosf(3 * ImGui::GetTime()*0.1f + 3.14f*(float)i / RES);
				aabbsData[j * RES + i] = D3D12_RAYTRACING_AABB{
					x, y, 0.0f, x + 3.5f / RES, y + 3.5f / RES, 1.0f / RES
				};
			}
		// Copies a buffer written using an initializer_list
		manager	gCopy PtrData(aabbs, aabbsData);

		manager gCopy ListData(screenVertices, {
			float2(-1, 1),
			float2(1, 1),
			float2(1, -1),

			float2(1, -1),
			float2(-1, -1),
			float2(-1, 1),
			});
	}

	void UploadData2(gObj<CopyingManager> manager) {
		D3D12_RAYTRACING_AABB* aabbsData = new D3D12_RAYTRACING_AABB[RES*RES];
		for (int j = 0; j < RES; j++)
			for (int i = 0; i < RES; i++)
			{
				float x = -0.5 + i * 1.0f / RES + 0.5f*sinf(ImGui::GetTime()*0.1f + 3.14f*(float)j / RES);
				float y = -0.5 + j * 1.0f / RES + 0.5f*cosf(3 * ImGui::GetTime()*0.1f + 31.14f*(float)i / RES);
				float z = sinf(ImGui::GetTime()*0.1f + 3.14f*(float)j / RES);
				aabbsData[j * RES + i] = D3D12_RAYTRACING_AABB{
					x, y, z, x + 10.5f / RES, y + 10.5f / RES, z + 1.0f / RES
				};
			}
		// Copies a buffer written using an initializer_list
		manager	gCopy PtrData(aabbs, aabbsData);
	}

	void Frame() {
		perform(UpdateBoxes);

		//flush_all_to_gpu;

		perform(UpdateADS);

		//flush_all_to_gpu;

		perform(Raytracing);
	}

	void UpdateBoxes(gObj<GraphicsManager> manager) {
		manager gSet Pipeline(updatePipeline);
		manager gSet Viewport(RES, RES);
		manager gSet VertexBuffer(screenVertices);

		manager gDispatch Triangles(6);
	}

	void UpdateADS(gObj<DXRManager> manager) {
		//UploadData2(manager);

		auto updatingGeometries = manager gCreate ProceduralGeometries(geometriesOnGPU);
		updatingGeometries->PrepareBuffer(aabbs);
		updatingGeometries gSet AABBs(aabbs);
		updatingGeometries gLoad Geometry(0, RES*RES);
		geometriesOnGPU = updatingGeometries gCreate UpdatedGeometry();

		auto updatingScene = manager gCreate Instances(this->Scene);
		updatingScene gLoad Instance(geometriesOnGPU);
		this->Scene = updatingScene gCreate UpdatedScene();
	}

	void Raytracing(gObj<DXRManager> manager) {
		auto rtProgram = pipeline->_Program;
		rtProgram->Output = rtRenderTarget;
		rtProgram->Scene = this->Scene;
		manager gSet Pipeline(pipeline);
		manager gSet Program(rtProgram);
		manager gSet Miss(pipeline->MyMissShader, 0);
		for (int i = 0; i < RES; i++)
			for (int j = 0; j < RES; j++)
				manager gSet HitGroup(rtProgram->ClosestHit, i * RES + j);
		manager gSet RayGeneration(pipeline->MyRaygenShader);
		manager gDispatch Rays(render_target->Width, render_target->Height);
		manager gCopy All(render_target, rtRenderTarget);
	}
};