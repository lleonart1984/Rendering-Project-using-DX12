#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"
#include "../CommonRT/DirectLightingTechnique.h"

struct PPMTechnique : public DirectLightingTechnique {
public:

#include "DXR_PhotonTracing_Pipeline.h"

	// DXR pipeline for raytracing tracing stage to collect hit points in PPM
	struct DXR_RT_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> RTMainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> RTScattering;
		gObj<HitGroupHandle> RTMaterial;

		class DXR_RT_IL : public DXIL_Library<DXR_RT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PPMRaytracingStage_RT.cso"));

				_ gLoad Shader(Context()->RTMainRays, L"RTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->RTScattering, L"RTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_RT_Pipeline> {
			void Setup() {
				_ gSet Payload(4 * (3 + 1)); // 3- Accum, 1- Bounces
				_ gSet StackSize(RAY_TRACING_MAX_BOUNCES);
				_ gSet MaxHitGroupIndex(1000); // 1000 == max number of geometries
				_ gLoad Shader(Context()->RTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->RTMaterial, Context()->RTScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;

			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;

			// GBuffer Information
			gObj<Texture2D> Positions;
			gObj<Texture2D> Normals;
			gObj<Texture2D> Coordinates;
			gObj<Texture2D> MaterialIndices;

			gObj<Texture2D>* Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
				int2 padding;
			} CurrentObjectInfo;

			gObj<Buffer> RTHits;
			gObj<Texture2D> ScreenHead;
			gObj<Buffer> ScreenNext;
			gObj<Buffer> Malloc;
			gObj<Buffer> PM;
			gObj<Buffer> Radii;

			void Globals() {
				UAV(0, RTHits);
				UAV(1, ScreenHead);
				UAV(2, ScreenNext);
				UAV(3, Malloc);
				UAV(4, PM);
				UAV(5, Radii);

				ADS(0, Scene);
				SRV(1, Vertices);
				SRV(2, Materials);
				SRV(3, Positions);
				SRV(4, Normals);
				SRV(5, Coordinates);
				SRV(6, MaterialIndices);
				SRV_Array(7, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
			}

			void HitGroup_Locals() {
				CBV(2, CurrentObjectInfo);
			}
		};
		gObj<DXR_RT_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};

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

	struct Scheduling : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PPMScheduler_CS.cso"));
		}

		gObj<Buffer> RTHits;
		gObj<Buffer> Photons;
		gObj<Buffer> Radii;

		gObj<Texture2D> ScreenHead;
		gObj<Buffer> ScreenNext;

		gObj<Buffer> GridHead;
		gObj<Buffer> GridNext;

		gObj<Buffer> PM;

		void Globals() {
			UAV(0, PM, ShaderType_Any);

			SRV(0, RTHits, ShaderType_Any);
			SRV(1, Photons, ShaderType_Any);
			SRV(2, Radii, ShaderType_Any);
			SRV(3, ScreenHead, ShaderType_Any);
			SRV(4, ScreenNext, ShaderType_Any);
			SRV(5, GridHead, ShaderType_Any);
			SRV(6, GridNext, ShaderType_Any);
		}
	};

	struct MergingPM : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PPMMergeMaps_CS.cso"));
		}

		gObj<Buffer> TPM;
		gObj<Buffer> Radii;

		gObj<Texture2D> ScreenHead;
		gObj<Buffer> ScreenNext;
		gObj<Buffer> WPM;

		void Globals() {
			SRV(0, ScreenHead, ShaderType_Any);
			SRV(1, ScreenNext, ShaderType_Any);
			SRV(2, WPM, ShaderType_Any);

			UAV(0, TPM, ShaderType_Any);
			UAV(1, Radii, ShaderType_Any);
		}
	};

	struct Accumulating : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PPMAccumulateRadiance_CS.cso"));
		}

		gObj<Buffer> RTHits;
		gObj<Buffer> TPM;
		gObj<Buffer> Radii;

		gObj<Texture2D> ScreenHead;
		gObj<Buffer> ScreenNext;

		gObj<Buffer> Materials;
		gObj<Texture2D>* Textures;
		int TextureCount;
		
		gObj<Texture2D> Background;
		gObj<Texture2D> Output;

		void Globals() {
			SRV(0, RTHits, ShaderType_Any);
			SRV(1, TPM, ShaderType_Any);
			SRV(2, Radii, ShaderType_Any);
			SRV(3, ScreenHead, ShaderType_Any);
			SRV(4, ScreenNext, ShaderType_Any);

			SRV(5, Materials, ShaderType_Any);
			SRV(6, Background, ShaderType_Any);
			
			SRV_Array(7, Textures, TextureCount, ShaderType_Any);

			Static_SMP(0, Sampler::Linear(), ShaderType_Any);

			UAV(0, Output, ShaderType_Any);
		}
	};


	gObj<DXR_RT_Pipeline> dxrRTPipeline;
	gObj<DXR_PT_Pipeline> dxrPTPipeline;
	gObj<PhotonMapConstructionPipeline> constructing;
	gObj<Scheduling> scheduling;
	gObj<MergingPM> merging;
	gObj<Accumulating> accumulating;

	// Baked scene geometries used for toplevel instance updates
	gObj<GeometriesOnGPU> sceneGeometriesOnGPU;
	// Vertex buffer for scene triangles
	gObj<Buffer> VB;

	void Startup() {

		DirectLightingTechnique::Startup();

		flush_all_to_gpu;

		_ gLoad Pipeline(dxrPTPipeline);
		_ gLoad Pipeline(dxrRTPipeline);
		_ gLoad Pipeline(constructing);
		_ gLoad Pipeline(scheduling);
		_ gLoad Pipeline(merging);
		_ gLoad Pipeline(accumulating);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		flush_all_to_gpu;

		perform(CreateSceneOnGPU);
	}

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"

	struct RayHitInfo {
		// Vertex
		float3 P;
		float3 N;
		float2 C;
		float3 T;
		float3 B;

		// Ray info
		float2 Grad;
		float3 D;
		float3 I;
		int MI;
	};

	struct RayHitAccum {
		float N;
		float3 Accum;
	};

	void CreatingAssets(gObj<DXRManager> manager) {

#pragma region DXR Photon trace Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrPTPipeline->_Program->Positions = gBufferFromLight->pipeline->GBuffer_P;
		dxrPTPipeline->_Program->Normals = gBufferFromLight->pipeline->GBuffer_N;
		dxrPTPipeline->_Program->Coordinates = gBufferFromLight->pipeline->GBuffer_C;
		dxrPTPipeline->_Program->MaterialIndices = gBufferFromLight->pipeline->GBuffer_M;

		dxrPTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->LightingCB = computeDirectLighting->Lighting;
		//dxrPTPipeline->_Program->ProgressivePass = _ gCreate ConstantBuffer<int>();
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		dxrPTPipeline->_Program->RadiusFactors = _ gCreate RWStructuredBuffer<float>(PHOTON_DIMENSION * PHOTON_DIMENSION);
#pragma endregion

#pragma region DXR Ray trace gathering hit points Pipeline Objects
		dxrRTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrRTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrRTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrRTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrRTPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrRTPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrRTPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrRTPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;

		// CBs will be updated every frame
		dxrRTPipeline->_Program->CameraCB = computeDirectLighting->ViewTransform;
		dxrRTPipeline->_Program->LightingCB = computeDirectLighting->Lighting;

		int MaxNumberOfHits = (render_target->Width * render_target->Height) * (1 + (1 << RAY_TRACING_MAX_BOUNCES));

		dxrRTPipeline->_Program->RTHits = _ gCreate RWStructuredBuffer<RayHitInfo>(MaxNumberOfHits);
		dxrRTPipeline->_Program->ScreenHead = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);
		dxrRTPipeline->_Program->ScreenNext = _ gCreate RWStructuredBuffer<int>(MaxNumberOfHits);
		dxrRTPipeline->_Program->Malloc = _ gCreate RWStructuredBuffer<int>(1);
		dxrRTPipeline->_Program->PM = _ gCreate RWStructuredBuffer<RayHitAccum>(MaxNumberOfHits);
		dxrRTPipeline->_Program->Radii = _ gCreate RWStructuredBuffer<float>(MaxNumberOfHits);
#pragma endregion

#pragma region Photon Map Construction

		constructing->Photons = dxrPTPipeline->_Program->Photons;

#if USE_VOLUME_GRID
		constructing->HeadBuffer = _ gCreate RWStructuredBuffer<int>(PHOTON_GRID_SIZE * PHOTON_GRID_SIZE * PHOTON_GRID_SIZE);
#else
		constructing->HeadBuffer = _ gCreate RWStructuredBuffer<int>(HASH_CAPACITY);
#endif
		constructing->NextBuffer = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);

#pragma endregion

#pragma region Scheduling
		scheduling->Photons = dxrPTPipeline->_Program->Photons;

		scheduling->RTHits = dxrRTPipeline->_Program->RTHits;
		scheduling->Radii = dxrRTPipeline->_Program->Radii;
		scheduling->ScreenHead = dxrRTPipeline->_Program->ScreenHead;
		scheduling->ScreenNext = dxrRTPipeline->_Program->ScreenNext;
		scheduling->GridHead = constructing->HeadBuffer;
		scheduling->GridNext = constructing->NextBuffer;
		scheduling->PM = _ gCreate RWStructuredBuffer<RayHitAccum>(MaxNumberOfHits);
#pragma endregion

#pragma region Merging
		merging->ScreenHead	= dxrRTPipeline->_Program->ScreenHead;
		merging->ScreenNext	= dxrRTPipeline->_Program->ScreenNext;
		merging->TPM		= dxrRTPipeline->_Program->PM;
		merging->Radii		= dxrRTPipeline->_Program->Radii;
		merging->WPM		= scheduling->PM;
#pragma endregion



#pragma region Accumulating

		accumulating->RTHits = dxrRTPipeline->_Program->RTHits;
		accumulating->TPM = dxrRTPipeline->_Program->PM;
		accumulating->Radii = dxrRTPipeline->_Program->Radii;
		accumulating->ScreenHead = dxrRTPipeline->_Program->ScreenHead;
		accumulating->ScreenNext = dxrRTPipeline->_Program->ScreenNext;

		accumulating->TextureCount = sceneLoader->TextureCount;
		accumulating->Textures = sceneLoader->Textures;
		accumulating->Materials = sceneLoader->MaterialBuffer;

		accumulating->Background = DirectLighting;

		accumulating->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);

#pragma endregion

	}

	void CreateSceneOnGPU(gObj<DXRManager> manager) {
		/// Loads a static scene for further ray-tracing

		// load full vertex buffer of all scene geometries
		VB = _ gCreate GenericBuffer<SCENE_VERTEX>(D3D12_RESOURCE_STATE_GENERIC_READ, Scene->VerticesCount(), CPU_WRITE_GPU_READ);
		manager gCopy PtrData(VB, Scene->Vertices());

		auto sceneGeometriesBuilder = manager gCreate TriangleGeometries();
		sceneGeometriesBuilder gSet VertexBuffer(VB, SCENE_VERTEX::Layout());
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{ // Create a geometry for each obj loaded group
			auto sceneObj = Scene->Objects()[i];
			sceneGeometriesBuilder gLoad Geometry(sceneObj.startVertex, sceneObj.vertexesCount);
		}
		sceneGeometriesOnGPU = sceneGeometriesBuilder gCreate BakedGeometry();

		auto sceneInstances = manager gCreate Instances();
		sceneInstances gLoad Instance(sceneGeometriesOnGPU);
		dxrRTPipeline->_Program->Scene =
		dxrPTPipeline->_Program->Scene = sceneInstances gCreate BakedScene();
	}

	void Frame() {

		DirectLightingTechnique::Frame();

		perform(Raytracing);

		perform(Photontracing);

		perform(AccumulateFinal);
	}
	
	int WaveIndex = 0;

	void Photontracing(gObj<DXRManager> manager) {

		auto ptRTProgram = dxrPTPipeline->_Program;
		manager gCopy ValueData(ptRTProgram->CameraCB, lightView.getInverse());

		for (int i = 0; i < PHOTON_WAVES; i++) { // each PPM wave of photons

			ptRTProgram->ProgressivePass = i;

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

				firstTime = false;
			}

			manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

			manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION);

#pragma endregion

			auto cmanager = manager.Dynamic_Cast<ComputeManager>();

			// Create Photon map grid
			cmanager gClear UAV(constructing->HeadBuffer, (unsigned int)-1);
			cmanager gSet Pipeline(constructing);
			cmanager gDispatch Threads(PHOTON_DIMENSION * PHOTON_DIMENSION / CS_1D_GROUPSIZE);

			// Setup gathering per screen hash index of ray hits
			cmanager gSet Pipeline(scheduling);
			cmanager gDispatch Threads(
				(int)ceil(render_target->Width / (double)CS_2D_GROUPSIZE),
				(int)ceil(render_target->Height / (double)CS_2D_GROUPSIZE)
			);

			// Merge TPM and WPM
			cmanager gSet Pipeline(merging);
			cmanager gDispatch Threads(
				(int)ceil(render_target->Width / (double)CS_2D_GROUPSIZE),
				(int)ceil(render_target->Height / (double)CS_2D_GROUPSIZE)
			);
		}
	}

	void Raytracing(gObj<DXRManager> manager) {
		auto rtProgram = dxrRTPipeline->_Program;

		// Set DXR Pipeline
		manager gSet Pipeline(dxrRTPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		manager gClear UAV(dxrRTPipeline->_Program->Malloc, 0u);
		manager gClear UAV(dxrRTPipeline->_Program->ScreenHead, (unsigned int)-1);

		int startTriangle;

#pragma region Raytrace Stage

		static bool firstTime = true;

		if (firstTime) {
			// Set Environment Miss in slot 0
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
#pragma endregion
	}

	void AddingPhotons(gObj<DXRManager> dxrmanager) {
		
	}

	void AccumulateFinal(gObj<DXRManager> dxrmanager) {
		auto manager = dxrmanager.Dynamic_Cast<ComputeManager>();
		
		// Accumulating final colors
		manager gSet Pipeline(accumulating);
		manager gDispatch Threads(
			(int)ceil(render_target->Width / (double)CS_2D_GROUPSIZE),
			(int)ceil(render_target->Height / (double)CS_2D_GROUPSIZE)
		);

		// Copy DXR output texture to the render target
		manager gCopy All(render_target, accumulating->Output);
	}
};