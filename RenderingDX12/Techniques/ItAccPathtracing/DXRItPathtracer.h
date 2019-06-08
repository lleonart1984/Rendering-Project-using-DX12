#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"

struct DXRItPathtracer : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

	~DXRItPathtracer() {
	}

#define SHADOWMAP_DIMENSION 2048

	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;
	// GBuffer process used to build GBuffer data from light
	gObj<GBufferConstruction> gBufferFromLight;
	// GBuffer process used to build GBuffer data from viewer
	gObj<GBufferConstruction> gBufferFromViewer;

	// DXR pipeline for pathtracing stage
	struct DXR_PT_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> PTMainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> PTScattering;
		gObj<HitGroupHandle> PTMaterial;

		class DXR_RT_IL : public DXIL_Library<DXR_PT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\ItAccPathtracing\\ItAccPathtracer_RT.cso"));

				_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->PTScattering, L"PTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_PT_Pipeline> {
			void Setup() {
				_ gSet Payload(4 * 3 * 4); // 4 float3
				_ gSet StackSize(1); // No recursion needed!
				_ gLoad Shader(Context()->PTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->PTMaterial, Context()->PTScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;

			// GBuffer Information
			gObj<Texture2D> Positions;
			gObj<Texture2D> Normals;
			gObj<Texture2D> Coordinates;
			gObj<Texture2D> MaterialIndices;
			// GBuffer from light for visibility test during direct lighting
			gObj<Texture2D> LightPositions;

			gObj<Texture2D> *Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			gObj<Buffer> LightTransforms;
			gObj<Buffer> PathtracingInfo;

			gObj<Texture2D> Output;


			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
			} CurrentObjectInfo;

			void Globals() {
				UAV(0, Output);

				ADS(0, Scene);
				SRV(1, Vertices);
				SRV(2, Materials);

				SRV(3, Positions);
				SRV(4, Normals);
				SRV(5, Coordinates);
				SRV(6, MaterialIndices);

				SRV(7, LightPositions);

				SRV_Array(8, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, LightTransforms);
				CBV(3, PathtracingInfo);
			}

			void HitGroup_Locals() {
				CBV(4, CurrentObjectInfo);
			}
		};
		gObj<DXR_RT_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};

	gObj<Buffer> screenVertices;
	gObj<DXR_PT_Pipeline> dxrPTPipeline;

	void SetScene(gObj<CA4G::Scene> scene) {
		IHasScene::SetScene(scene);
		if (sceneLoader != nullptr)
			sceneLoader->SetScene(scene);
	}

	void Startup() {

		// Load and setup scene loading process
		sceneLoader = new RetainedSceneLoader();
		sceneLoader->SetScene(this->Scene);
		_ gLoad Subprocess(sceneLoader);

		// Load and setup gbuffer construction process from light
		gBufferFromLight = new GBufferConstruction(SHADOWMAP_DIMENSION, SHADOWMAP_DIMENSION);
		gBufferFromLight->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromLight);

		// Load and setup gbuffer construction process from viewer
		gBufferFromViewer = new GBufferConstruction(render_target->Width, render_target->Height);
		gBufferFromViewer->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromViewer);

		wait_for(signal(flush_all_to_gpu));

		_ gLoad Pipeline(dxrPTPipeline);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		perform(CreateSceneOnGPU);
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

#pragma region DXR Pathtracing Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;

		// CBs will be updated every frame
		dxrPTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPTPipeline->_Program->LightTransforms = _ gCreate ConstantBuffer <Globals>();
		dxrPTPipeline->_Program->PathtracingInfo = _ gCreate ConstantBuffer <int>();

		dxrPTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
#pragma endregion

	}

	gObj<Buffer> VB;
	void CreateSceneOnGPU(gObj<DXRManager> manager) {
		/// Loads a static scene for further ray-tracing

		// load full vertex buffer of all scene geometries
		VB = _ gCreate GenericBuffer<SCENE_VERTEX>(D3D12_RESOURCE_STATE_GENERIC_READ, Scene->VerticesCount(), CPU_WRITE_GPU_READ);
		manager gCopy PtrData(VB, Scene->Vertices());

		auto geometries = manager gCreate TriangleGeometries();
		geometries gSet VertexBuffer(VB, SCENE_VERTEX::Layout());
		//geometries gLoad Geometry(0, VB->ElementCount);
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{ // Create a geometry for each obj loaded group
			auto sceneObj = Scene->Objects()[i];
			geometries gLoad Geometry(sceneObj.startVertex, sceneObj.vertexesCount);
		}
		gObj<GeometriesOnGPU> geometriesOnGPU;
		geometriesOnGPU = geometries gCreate BakedGeometry();

		// Creates a single instance to refer all static objects in bottom level acc ds.
		auto instances = manager gCreate Instances();
		instances gLoad Instance(geometriesOnGPU);
		dxrPTPipeline->_Program->Scene = instances gCreate BakedScene();
	}

	float4x4 view, proj;
	float4x4 lightView, lightProj;

	void Frame() {
		if (CameraIsDirty) {
#pragma region Construct GBuffer from viewer
			Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

			gBufferFromViewer->ViewMatrix = view;
			gBufferFromViewer->ProjectionMatrix = proj;
			ExecuteFrame(gBufferFromViewer);
#pragma endregion
		}

#pragma region Construct GBuffer from light
		lightView = LookAtLH(this->Light->Position, this->Light->Position + float3(0, -1, 0), float3(0, 0, 1));
		lightProj = PerspectiveFovLH(PI / 2, 1, 0.001f, 10);
		gBufferFromLight->ViewMatrix = lightView;
		gBufferFromLight->ProjectionMatrix = lightProj;
		ExecuteFrame(gBufferFromLight);
#pragma endregion

		perform(Raytracing);

		return;

		//		wait_for(signal(flush_all_to_gpu));
	}

	void Raytracing(gObj<DXRManager> manager) {

		static int FrameIndex = 0;

		auto rtProgram = dxrPTPipeline->_Program;
		
		if (CameraIsDirty)
		{
			FrameIndex = 0;
			manager gClear UAV(rtProgram->Output, float4(0, 0, 0, 0));
		}

		// Update camera
		// Required during ray-trace stage
		manager gCopy ValueData(rtProgram->CameraCB, view.getInverse());

		manager gCopy ValueData(rtProgram->LightTransforms, Globals{
				lightProj,
				lightView
			});

		// Update lighting needed for photon tracing
		manager gCopy ValueData(rtProgram->LightingCB, Lighting{
			Light->Position, 0,
			Light->Intensity, 0
			});

		manager gCopy ValueData(rtProgram->PathtracingInfo, FrameIndex);

		dxrPTPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrPTPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrPTPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrPTPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		dxrPTPipeline->_Program->LightPositions = gBufferFromLight->pipeline->GBuffer_P;

		// Set DXR Pipeline
		manager gSet Pipeline(dxrPTPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Single Pathtrace Stage

		// Set Miss in slot 0
		manager gSet Miss(dxrPTPipeline->EnvironmentMap, 0);

		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			manager gSet HitGroup(dxrPTPipeline->PTMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

		// Dispatch primary rays
		manager gDispatch Rays(render_target->Width, render_target->Height);

		// Copy DXR output texture to the render target
		manager gCopy All(render_target, rtProgram->Output);

		FrameIndex++;
#pragma endregion
	}

};