#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"

struct BPP_PhotonMap3Technique : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;
	// GBuffer process used to build GBuffer data from light
	gObj<GBufferConstruction> gBufferFromLight;
	// GBuffer process used to build GBuffer data from viewer
	gObj<GBufferConstruction> gBufferFromViewer;

#include "DXR_PhotonTracing_Pipeline.h"

	// DXR pipeline for Morton indexing and permutation initialization (should be a compute shader)
	// needs to be fixed asap!
	struct DXR_Morton_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> Main;

		class DXR_PM_IL : public DXIL_Library<DXR_Morton_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PhotonMortonIndexing_RT.cso"));
				_ gLoad Shader(Context()->Main, L"Main");
			}
		};
		gObj<DXR_PM_IL> _Library;

		struct DXR_PM_Program : public RTProgram<DXR_Morton_Pipeline> {
			void Setup() {
				_ gLoad Shader(Context()->Main);
			}

			gObj<Buffer> Photons;
			gObj<Buffer> Indices;

			void Globals() {
				UAV(0, Indices);

				SRV(0, Photons);
			}
		};
		gObj<DXR_PM_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};

	struct BitonicStage {
		int len;
		int dif;
	};

	// DXR pipeline for Morton indexing and permutation initialization (should be a compute shader)
	// needs to be fixed asap!
	struct DXR_Sorting_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> Main;

		class DXR_PM_IL : public DXIL_Library<DXR_Sorting_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PhotonBitonicSort_RT.cso"));
				_ gLoad Shader(Context()->Main, L"Main");
			}
		};
		gObj<DXR_PM_IL> _Library;

		struct DXR_PM_Program : public RTProgram<DXR_Sorting_Pipeline> {
			void Setup() {
				_ gLoad Shader(Context()->Main);
			}

			gObj<Buffer> Photons;
			gObj<Buffer> Indices;

			// CB views
			BitonicStage Stage;

			void Globals() {
				UAV(0, Photons);
				UAV(1, Indices);
				CBV(0, Stage);
			}
		};
		gObj<DXR_PM_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};


	// DXR pipeline for AABB construction and Radii population (should be a compute shader)
	// needs to be fixed asap!
	struct DXR_PM_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> Main;

		class DXR_PM_IL : public DXIL_Library<DXR_PM_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\BPP_PhotonMapMortonConstruction_RT.cso"));
				_ gLoad Shader(Context()->Main, L"Main");
			}
		};
		gObj<DXR_PM_IL> _Library;

		struct DXR_PM_Program : public RTProgram<DXR_PM_Pipeline> {
			void Setup() {
				_ gLoad Shader(Context()->Main);
			}

			gObj<Buffer> Photons;
			gObj<Buffer> AABBs;
			gObj<Buffer> Radii;
			gObj<Buffer> MortonIndices;

			void Globals() {
				UAV(0, AABBs);
				UAV(1, Radii);

				SRV(0, Photons);
				SRV(1, MortonIndices);
			}
		};
		gObj<DXR_PM_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};

	// DXR pipeline for photon gathering stage
	struct DXR_RT_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> RTMainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> RTScattering;
		gObj<HitGroupHandle> RTMaterial;

		gObj<MissHandle> PhotonGatheringMiss;
		gObj<IntersectionHandle> PhotonGatheringIntersection;
		gObj<AnyHitHandle> PhotonGatheringAnyHit;
		gObj<HitGroupHandle> PhotonGatheringMaterial;

		class DXR_RT_IL : public DXIL_Library<DXR_RT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\BPP_PhotonGather2_RT.cso"));

				_ gLoad Shader(Context()->RTMainRays, L"RTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->RTScattering, L"RTScattering");

				_ gLoad Shader(Context()->PhotonGatheringAnyHit, L"PhotonGatheringAnyHit");
				_ gLoad Shader(Context()->PhotonGatheringIntersection, L"PhotonGatheringIntersection");
				_ gLoad Shader(Context()->PhotonGatheringMiss, L"PhotonGatheringMiss");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_RT_Pipeline> {
			void Setup() {
				_ gSet Payload(4 * (3 + 3)); // 3- Normal, 3- Accum
				_ gSet StackSize(RAY_TRACING_MAX_BOUNCES + 1); //  +1 is due to the last trace function call for photon gathering
				_ gSet MaxHitGroupIndex(1 + 1000); // 1 == hitgroup index for photons, 1000 == max number of geometries
				_ gLoad Shader(Context()->RTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gLoad Shader(Context()->PhotonGatheringMiss);
				_ gCreate HitGroup(Context()->RTMaterial, Context()->RTScattering, nullptr, nullptr);
				_ gCreate HitGroup(Context()->PhotonGatheringMaterial, nullptr, Context()->PhotonGatheringAnyHit, Context()->PhotonGatheringIntersection);
			}

			gObj<SceneOnGPU> SceneAndPhotonMap;
			gObj<Buffer> Photons;
			gObj<Buffer> Radii;

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
			gObj<Buffer> ProgressiveCB;
			gObj<Buffer> LightTransforms;

			gObj<Texture2D> Output;

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
				int2 padding;
			} CurrentObjectInfo;

			void Globals() {
				UAV(0, Output);

				ADS(0, SceneAndPhotonMap);

				SRV(1, Photons);

				SRV(2, Vertices);
				SRV(3, Materials);

				SRV(4, Positions);
				SRV(5, Normals);
				SRV(6, Coordinates);
				SRV(7, MaterialIndices);

				SRV(8, LightPositions);

				SRV(9, Radii);

				SRV_Array(10, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, ProgressiveCB);
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
	gObj<DXR_Morton_Pipeline> dxrMortonPipeline;
	gObj<DXR_Sorting_Pipeline> dxrSortingPipeline;
	gObj<DXR_PM_Pipeline> dxrPMPipeline;
	gObj<DXR_RT_Pipeline> dxrRTPipeline;

	// AABBs buffer for photon map
	gObj<Buffer> PhotonsAABBs;
	// Baked photon map used for updates
	gObj<GeometriesOnGPU> PhotonsAABBsOnTheGPU [PHOTONS_LOD + 1];
	// Baked scene geometries used for toplevel instance updates
	gObj<GeometriesOnGPU> sceneGeometriesOnGPU;
	// Vertex buffer for scene triangles
	gObj<Buffer> VB;

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
		gBufferFromLight = new GBufferConstruction(PHOTON_DIMENSION, PHOTON_DIMENSION);
		gBufferFromLight->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromLight);

		// Load and setup gbuffer construction process from viewer
		gBufferFromViewer = new GBufferConstruction(render_target->Width, render_target->Height);
		gBufferFromViewer->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromViewer);

		flush_all_to_gpu;

		_ gLoad Pipeline(dxrPTPipeline);
		_ gLoad Pipeline(dxrMortonPipeline);
		_ gLoad Pipeline(dxrSortingPipeline);
		_ gLoad Pipeline(dxrPMPipeline);
		_ gLoad Pipeline(dxrRTPipeline);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		flush_all_to_gpu;

		perform(CreateSceneOnGPU);
	}

#define PHOTON_WITH_DIRECTION
#define PHOTON_WITH_NORMAL
#define PHOTON_WITH_POSITION
#include "PhotonDefinition.h"

	double doubleRand() {
		return double(rand()) / (double(RAND_MAX) + 1.0);
	}

	void CreatingAssets(gObj<CopyingManager> manager) {
		// Photons aabbs used in bottom level structure building and updates
		PhotonsAABBs = _ gCreate RWAccelerationDatastructureBuffer<D3D12_RAYTRACING_AABB>(PHOTON_DIMENSION*PHOTON_DIMENSION);

#pragma region DXR Photon trace Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;

		// CBs will be updated every frame
		dxrPTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPTPipeline->_Program->ProgressivePass = _ gCreate ConstantBuffer<int>();
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(PHOTON_DIMENSION*PHOTON_DIMENSION);
#pragma endregion

#pragma region DXR Morton indexing and initialization
		dxrMortonPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		dxrMortonPipeline->_Program->Indices = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION*PHOTON_DIMENSION);
#pragma endregion

#pragma region DXR Sorting with Bitonic
		dxrSortingPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		dxrSortingPipeline->_Program->Indices = dxrMortonPipeline->_Program->Indices;
#pragma endregion

#pragma region DXR Pipeline for PM construction using AABBs
		dxrPMPipeline->_Program->AABBs = PhotonsAABBs;
		dxrPMPipeline->_Program->Radii = _ gCreate RWStructuredBuffer<float>(PHOTON_DIMENSION*PHOTON_DIMENSION);
		dxrPMPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		dxrPMPipeline->_Program->MortonIndices = dxrMortonPipeline->_Program->Indices;
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
		dxrRTPipeline->_Program->ProgressiveCB = dxrPTPipeline->_Program->ProgressivePass;
		dxrRTPipeline->_Program->LightingCB = dxrPTPipeline->_Program->LightingCB;

		dxrRTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		// Bind now Photon map as SRVs
		dxrRTPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		dxrRTPipeline->_Program->Radii = dxrPMPipeline->_Program->Radii;
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
		dxrPTPipeline->_Program->Scene = sceneInstances gCreate BakedScene();
	}



	void CreateSceneAndPhotonMapOnGPU(gObj<DXRManager> manager) {
		// Building top-level ADS for SceneAndPhotonMap
		auto sceneInstances = manager gCreate Instances();

		for (int t = 0; t <= PHOTONS_LOD; t++)
		{
			auto photonMapBuilder = manager gCreate ProceduralGeometries();
			photonMapBuilder gSet AABBs(PhotonsAABBs);
			int start = t == 0 ? 0 : (1 << (t - 1))*PHOTON_DIMENSION;
			int count = max(PHOTON_DIMENSION, start);
			photonMapBuilder gLoad Geometry(start, count);
			PhotonsAABBsOnTheGPU[t] = photonMapBuilder gCreate BakedGeometry(true, true);
			sceneInstances gLoad Instance(PhotonsAABBsOnTheGPU[t], 2, 0, start);
		}

		sceneInstances gLoad Instance(sceneGeometriesOnGPU, 1);

		dxrRTPipeline->_Program->SceneAndPhotonMap = sceneInstances gCreate BakedScene(true, true);
	}

	void UpdatePhotonMap(gObj<DXRManager> manager) {
		
		auto sceneInstances = manager gCreate Instances(dxrRTPipeline->_Program->SceneAndPhotonMap);
		
		for (int t = 0; t <= PHOTONS_LOD; t++)
		{
			auto photonMapBuilder = manager gCreate ProceduralGeometries(PhotonsAABBsOnTheGPU[t]);
			photonMapBuilder gSet AABBs(PhotonsAABBs);
			int start = t == 0 ? 0 : (1 << (t - 1))*PHOTON_DIMENSION;
			int count = max(PHOTON_DIMENSION, start);
			photonMapBuilder gLoad Geometry(start, count);
			PhotonsAABBsOnTheGPU[t] = photonMapBuilder gCreate RebuiltGeometry(true, true);
			sceneInstances gLoad Instance(PhotonsAABBsOnTheGPU[t], 2, 0, start);
		}

		sceneInstances gLoad Instance(sceneGeometriesOnGPU, 1);
		dxrRTPipeline->_Program->SceneAndPhotonMap = sceneInstances gCreate UpdatedScene();
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

		if (LightSourceIsDirty)
		{
#pragma region Construct GBuffer from light
			lightView = LookAtLH(this->Light->Position, this->Light->Position + float3(0, -1, 0), float3(0, 0, 1));
			lightProj = PerspectiveFovLH(PI / 2, 1, 0.001f, 10);
			gBufferFromLight->ViewMatrix = lightView;
			gBufferFromLight->ProjectionMatrix = lightProj;
			ExecuteFrame(gBufferFromLight);
#pragma endregion
		}

		perform(Photontracing);

		perform(MortonPhotons);

		perform(SortPhotons);

		perform(ConstructPhotonMap);

		static bool firstTime = true;
		if (firstTime) {
			perform(CreateSceneAndPhotonMapOnGPU);
			firstTime = false;
		}
		else
			perform(UpdatePhotonMap);

		perform(Raytracing);
	}

	int len; int dif;

	void Photontracing(gObj<DXRManager> manager) {

		auto ptRTProgram = dxrPTPipeline->_Program;

		static int FrameIndex = 0;

		if (CameraIsDirty)
			FrameIndex = 0;

		manager gCopy ValueData(ptRTProgram->ProgressivePass, FrameIndex);

		FrameIndex++;


		// Update lighting needed for photon tracing
		manager gCopy ValueData(ptRTProgram->LightingCB, Lighting{
			Light->Position, 0,
			Light->Intensity, 0
			});

		manager gCopy ValueData(ptRTProgram->CameraCB, lightView.getInverse());

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

			firstTime = false;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

		// Dispatch rays for more than 2^22 photons
		manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION);

#pragma endregion
	}

	void MortonPhotons(gObj<DXRManager> manager) {
		auto rtMortonProgram = dxrMortonPipeline->_Program;
		manager gSet Pipeline(dxrMortonPipeline);
		manager gSet Program(rtMortonProgram);
		manager gSet RayGeneration(dxrMortonPipeline->Main);
		manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION); // initialize photon indices (using morton curve) and permutation 0,1,2,...
	}

	void SortPhotons(gObj<DXRManager> manager) {
		auto rtSortingProgram = dxrSortingPipeline->_Program;
		manager gSet Pipeline(dxrSortingPipeline);

		int n = PHOTON_DIMENSION * PHOTON_DIMENSION;
		for (len = 2; len <= n; len <<= 1)
			for (dif = len >> 1; dif > 0; dif >>= 1)
			{
				rtSortingProgram->Stage = BitonicStage{ len, dif };
				manager gSet Program(rtSortingProgram);
				manager gSet RayGeneration(dxrSortingPipeline->Main);
				// Bitonic sort wave
				manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION / 2);
			}
	}

	void ConstructPhotonMap(gObj<DXRManager> manager) {
		auto rtProgram = dxrPMPipeline->_Program;

		manager gSet Pipeline(dxrPMPipeline);
		manager gSet Program(rtProgram);
		manager gSet RayGeneration(dxrPMPipeline->Main);
		manager gDispatch Rays(PHOTON_DIMENSION, PHOTON_DIMENSION);
	}

	void Raytracing(gObj<DXRManager> manager) {

		auto rtProgram = dxrRTPipeline->_Program;

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

		dxrRTPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrRTPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrRTPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrRTPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		dxrRTPipeline->_Program->LightPositions = gBufferFromLight->pipeline->GBuffer_P;

		if (CameraIsDirty)
			manager gClear UAV(rtProgram->Output, 0u);

		// Set DXR Pipeline
		manager gSet Pipeline(dxrRTPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Raytrace Stage

		static bool firstTime = true;

		if (firstTime) {
			// Set Environment Miss in slot 0
			manager gSet Miss(dxrRTPipeline->EnvironmentMap, 0);
			// Set PhotonGatheringMiss in slot 1
			manager gSet Miss(dxrRTPipeline->PhotonGatheringMiss, 1);

			// Set PhotonGatheringMaterial in ST slot 0
			manager gSet HitGroup(dxrRTPipeline->PhotonGatheringMaterial, 0);

			// Setup a simple hitgroup per object
			// each object knows the offset in triangle buffer
			// and the material index for further light scattering
			startTriangle = 0;
			for (int i = 0; i < Scene->ObjectsCount(); i++)
			{
				auto sceneObject = Scene->Objects()[i];

				rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
				rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

				manager gSet HitGroup(dxrRTPipeline->RTMaterial, i + 1);

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