#pragma once

#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"

struct GridPhotonMapTechnique : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

	~GridPhotonMapTechnique() {
	}

	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;
	// GBuffer process used to build GBuffer data from light
	gObj<GBufferConstruction> gBufferFromLight;
	// GBuffer process used to build GBuffer data from viewer
	gObj<GBufferConstruction> gBufferFromViewer;

#include "DXR_PhotonTracing_Pipeline.h"

	struct PhotonMapConstructionPipeline : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\GridPhotonMapConstruction_CS.cso"));
		}
		
		gObj<Buffer> Photons;
		gObj<Buffer> HeadBuffer;
		gObj<Buffer> NextBuffer;

		void Globals() {
			SRV(0, Photons, ShaderType_Any);
			UAV(0, HeadBuffer, ShaderType_Any);
			UAV(1, NextBuffer, ShaderType_Any);
		}
	};

	// DXR pipeline for photon gathering stage
	struct DXR_RT_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> RTMainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> RTScattering;
		gObj<HitGroupHandle> RTMaterial;

		class DXR_RT_IL : public DXIL_Library<DXR_RT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\GridPhotonGather_RT.cso"));

				_ gLoad Shader(Context()->RTMainRays, L"RTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->RTScattering, L"RTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_RT_Pipeline> {
			void Setup() {
				_ gSet Payload(16);
				_ gSet StackSize(RAY_TRACING_MAX_BOUNCES);
				_ gLoad Shader(Context()->RTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->RTMaterial, Context()->RTScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
			// Photon map binding objects (now as readonly-resources)
			gObj<Buffer> Photons; // Photon map in a lineal buffer
			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;

			// GBuffer Information
			gObj<Texture2D> Positions;
			gObj<Texture2D> Normals;
			gObj<Texture2D> Coordinates;
			gObj<Texture2D> MaterialIndices;
			// GBuffer from light for visibility test during direct lighting
			gObj<Texture2D> LightPositions;

			gObj<Buffer> HashtableBuffer; // head ptrs of linked lists
			gObj<Buffer> NextBuffer; // Reference to next element in each linked list node

			gObj<Texture2D>* Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			gObj<Buffer> LightTransforms;
			gObj<Buffer> ProgressivePass;

			gObj<Texture2D> Output;
			gObj<Texture2D> Accum;

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
			} CurrentObjectInfo;

			void Globals() {
				UAV(0, Output);
				UAV(1, Accum);

				ADS(0, Scene);
				SRV(1, Photons);
				SRV(2, Vertices);
				SRV(3, Materials);

				SRV(4, Positions);
				SRV(5, Normals);
				SRV(6, Coordinates);
				SRV(7, MaterialIndices);

				SRV(8, LightPositions);

				SRV(9, HashtableBuffer);
				SRV(10, NextBuffer);

				SRV_Array(11, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, ProgressivePass);
				CBV(3, LightTransforms);
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
	//gObj<DXR_PM_Pipeline> dxrPMPipeline;
	gObj<PhotonMapConstructionPipeline> constructingPM;
	gObj<DXR_RT_Pipeline> dxrRTPipeline;

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
		_ gLoad Pipeline(constructingPM);
		_ gLoad Pipeline(dxrRTPipeline);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		perform(CreateSceneOnGPU);
	}

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"

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

#pragma region DXR Photon trace Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;

		// CBs will be updated every frame
		dxrPTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(PHOTON_DIMENSION * PHOTON_DIMENSION);
#pragma endregion

#pragma region DXR Photon map construction pipeline objects

		constructingPM->Photons = dxrPTPipeline->_Program->Photons;
		//dxrPMPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
#if USE_VOLUME_GRID
		constructingPM->HeadBuffer = _ gCreate RWStructuredBuffer<int>(PHOTON_GRID_SIZE * PHOTON_GRID_SIZE * PHOTON_GRID_SIZE);
		//dxrPMPipeline->_Program->HeadBuffer = _ gCreate RWStructuredBuffer<int>(PHOTON_GRID_SIZE * PHOTON_GRID_SIZE * PHOTON_GRID_SIZE);
#else
		constructingPM->HeadBuffer = _ gCreate RWStructuredBuffer<int>(HASH_CAPACITY);
		//dxrPMPipeline->_Program->HeadBuffer = _ gCreate RWStructuredBuffer<int>(HASH_CAPACITY);
#endif
		constructingPM->NextBuffer = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		//dxrPMPipeline->_Program->NextBuffer = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);

#pragma endregion

#pragma region DXR Photon gathering Pipeline Objects
		dxrRTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrRTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrRTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrRTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;

		// CBs will be updated every frame
		dxrRTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrRTPipeline->_Program->LightTransforms = _ gCreate ConstantBuffer <Globals>();

		// Reused CBs from dxrPTPipeline
		dxrRTPipeline->_Program->LightingCB = dxrPTPipeline->_Program->LightingCB;

		dxrRTPipeline->_Program->ProgressivePass = dxrPTPipeline->_Program->ProgressivePass = _ gCreate ConstantBuffer<int>();

		dxrRTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		dxrRTPipeline->_Program->Accum = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		// Bind now as SRVs
		dxrRTPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		//dxrRTPipeline->_Program->HashtableBuffer = dxrPMPipeline->_Program->HeadBuffer;
		dxrRTPipeline->_Program->HashtableBuffer = constructingPM->HeadBuffer;
		//dxrRTPipeline->_Program->NextBuffer = dxrPMPipeline->_Program->NextBuffer;
		dxrRTPipeline->_Program->NextBuffer = constructingPM->NextBuffer;
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
		dxrPTPipeline->_Program->Scene = dxrRTPipeline->_Program->Scene = instances gCreate BakedScene();
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

		if (LightSourceIsDirty) {
#pragma region Construct GBuffer from light
			lightView = LookAtLH(this->Light->Position, this->Light->Position + float3(0, -1, 0), float3(0, 0, 1));
			lightProj = PerspectiveFovLH(PI / 2, 1, 0.001f, 10);
			gBufferFromLight->ViewMatrix = lightView;
			gBufferFromLight->ProjectionMatrix = lightProj;
			ExecuteFrame(gBufferFromLight);
#pragma endregion
		}

		perform(Photontracing);

		perform(PhotonMapConstruction);

		perform(Raytracing);
	}

	void Photontracing(gObj<DXRManager> manager) {

		static int FrameIndex = 0;

		if (CameraIsDirty || LightSourceIsDirty)
			FrameIndex = 0;

		manager gCopy ValueData(dxrPTPipeline->_Program->ProgressivePass, FrameIndex);

		FrameIndex++;

		auto ptRTProgram = dxrPTPipeline->_Program;

		if (LightSourceIsDirty) {
			// Update lighting needed for photon tracing
			manager gCopy ValueData(ptRTProgram->LightingCB, Lighting{
				Light->Position, 0,
				Light->Intensity, 0
				});
		}

		if (LightSourceIsDirty) {
			manager gCopy ValueData(ptRTProgram->CameraCB, lightView.getInverse());
		}

		dxrPTPipeline->_Program->Positions = gBufferFromLight->pipeline->GBuffer_P;
		dxrPTPipeline->_Program->Normals = gBufferFromLight->pipeline->GBuffer_N;
		dxrPTPipeline->_Program->Coordinates = gBufferFromLight->pipeline->GBuffer_C;
		dxrPTPipeline->_Program->MaterialIndices = gBufferFromLight->pipeline->GBuffer_M;

		// Set DXR Pipeline
		manager gSet Pipeline(dxrPTPipeline);
		// Activate program with main shaders
		manager gSet Program(ptRTProgram);

		int startTriangle;

#pragma region Photon Trace Stage
		static bool firstTime = true;

		if (firstTime) {
			// Set Miss in slot 0
			manager gSet Miss(dxrPTPipeline->PhotonMiss, 0);

			// Setup a simple hitgroup per object
			// each object knows the offset in triangle buffer
			// and the material index for further light scattering
			startTriangle = 0;
			for (int i = 0; i < Scene->ObjectsCount(); i++)
			{
				auto sceneObject = Scene->Objects()[i];

				ptRTProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
				ptRTProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

				manager gSet HitGroup(dxrPTPipeline->PhotonMaterial, i);

				startTriangle += sceneObject.vertexesCount / 3;
			}
			firstTime = true;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

		// Dispatch rays for 1 000 000 photons
		manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION);

#pragma endregion
	}

	void PhotonMapConstruction(gObj<DXRManager> manager) {
		auto computeManager = manager.Dynamic_Cast<ComputeManager>();
		computeManager gClear UAV(constructingPM->HeadBuffer, (unsigned int)-1); // reset head buffer to null for every list
		computeManager gSet Pipeline(constructingPM);
		computeManager gDispatch Threads(PHOTON_DIMENSION * PHOTON_DIMENSION / CS_1D_GROUPSIZE);
		
		//auto rtProgram = dxrPMPipeline->_Program;
		//manager gClear UAV(rtProgram->HeadBuffer, (unsigned int)-1); // reset head buffer to null for every list
		//manager gSet Pipeline(dxrPMPipeline);
		//manager gSet Program(rtProgram);
		//manager gSet RayGeneration(dxrPMPipeline->Main);
		//manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION);
	}

	void Raytracing(gObj<DXRManager> manager) {

		auto rtProgram = dxrRTPipeline->_Program;

		if (CameraIsDirty) {
			// Update camera
			// Required during ray-trace stage
			manager gCopy ValueData(rtProgram->CameraCB, view.getInverse());
		}

		if (LightSourceIsDirty) {
			manager gCopy ValueData(rtProgram->LightTransforms, Globals{
					lightProj,
					lightView
				});
		}

		dxrRTPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrRTPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrRTPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrRTPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		dxrRTPipeline->_Program->LightPositions = gBufferFromLight->pipeline->GBuffer_P;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			manager gClear UAV(rtProgram->Output, 0u);
			manager gClear UAV(rtProgram->Accum, 0u);
		}

		// Set DXR Pipeline
		manager gSet Pipeline(dxrRTPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Raytrace Stage
		static bool firstTime = true;

		if (firstTime) {
			// Set Miss in slot 0
			manager gSet Miss(dxrRTPipeline->EnvironmentMap, 0);

			// Setup a simple hitgroup per object
			// each object knows the offset in triangle buffer
			// and the material index for further light scattering
			startTriangle = 0;
			for (int i = 0; i < Scene->ObjectsCount(); i++)
			{
				auto sceneObject = Scene->Objects()[i];

				rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
				rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

				manager gSet HitGroup(dxrRTPipeline->RTMaterial, i);

				startTriangle += sceneObject.vertexesCount / 3;
			}
			firstTime = false;
		}
		// Setup a raygen shader
		manager gSet RayGeneration(dxrRTPipeline->RTMainRays);

		// Dispatch primary rays
		manager gDispatch Rays(render_target->Width, render_target->Height);

		// Copy DXR output texture to the render target
		manager gCopy All(render_target, rtProgram->Output);
#pragma endregion
	}

};