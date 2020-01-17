#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"
#include "../CommonRT/DirectLightingTechnique.h"

struct OctreePhotonMapTechnique : public DirectLightingTechnique {
public:

#include "DXR_PhotonTracing_Pipeline.h"

	struct MortonIndexing : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\BPP_MortonIndexing_CS.cso"));
		}

		gObj<Buffer> Photons;
		gObj<Buffer> Indices;
		gObj<Buffer> Permutation;

		void Globals() {
			UAV(0, Indices, ShaderType_Any);
			UAV(1, Permutation, ShaderType_Any);

			SRV(0, Photons, ShaderType_Any);
		}
	};

	struct BitonicStage {
		int len;
		int dif;
	};

	struct SortingPipeline : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PhotonBitonicSort_CS.cso"));
		}

		gObj<Buffer> Photons;
		gObj<Buffer> Indices;
		gObj<Buffer> Permutation;

		// CB views
		BitonicStage Stage;

		void Globals() {
			SRV(0, Photons, ShaderType_Any);
			SRV(1, Indices, ShaderType_Any);
			UAV(0, Permutation, ShaderType_Any);
		}

		void Locals() {
			CBV(0, Stage, ShaderType_Any);
		}
	};

	struct OctreeBuildPipeline : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\BuildOctree_CS.cso"));
		}

		gObj<Buffer> Indices;
		gObj<Buffer> Permutation;

		gObj<Buffer> LOD;
		gObj<Buffer> Skip;

		// CB views
		int BlockLOD;

		void Globals() {
			SRV(0, Indices, ShaderType_Any);
			SRV(1, Permutation, ShaderType_Any);
			UAV(0, LOD, ShaderType_Any);
			UAV(1, Skip, ShaderType_Any);
		}

		void Locals() {
			CBV(0, BlockLOD, ShaderType_Any);
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
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\GatherOctree_RT.cso"));

				_ gLoad Shader(Context()->RTMainRays, L"RTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->RTScattering, L"RTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_RT_Pipeline> {
			void Setup() {
				_ gSet Payload(4 * (3 + 3)); // 3- Normal, 3- Accum
				_ gSet StackSize(RAY_TRACING_MAX_BOUNCES);
				_ gSet MaxHitGroupIndex(1000); // 1000 == max number of geometries
				_ gLoad Shader(Context()->RTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->RTMaterial, Context()->RTScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
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

			gObj<Texture2D>* Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			int ProgressiveCB;
			gObj<Buffer> LightTransforms;
			
			gObj<Buffer> Morton;
			gObj<Buffer> Permutation;
			gObj<Buffer> LOD;
			gObj<Buffer> Skip;

			gObj<Texture2D> DirectLighting;
			gObj<Texture2D> Output;
			gObj<Texture2D> Accum;

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
				int2 padding;
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

				SRV(9, DirectLighting);

				SRV(10, Morton);
				SRV(11, Permutation);
				SRV(12, LOD);
				SRV(13, Skip);

				SRV_Array(14, Textures, TextureCount);

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

	gObj<DXR_PT_Pipeline> dxrPTPipeline;
	gObj<MortonIndexing> mortonIndexingPipeline;
	gObj<SortingPipeline> sortingPipeline;
	gObj<OctreeBuildPipeline> buildingOctree;
	gObj<DXR_RT_Pipeline> dxrRTPipeline;

	// AABBs buffer for photon map
	gObj<Buffer> PhotonsAABBs;
	// Baked photon map used for updates
	gObj<GeometriesOnGPU> PhotonsAABBsOnTheGPU;
	// Baked scene geometries used for toplevel instance updates
	gObj<GeometriesOnGPU> sceneGeometriesOnGPU;
	// Vertex buffer for scene triangles
	gObj<Buffer> VB;

	void Startup() {

		DirectLightingTechnique::Startup();

		flush_all_to_gpu;

		_ gLoad Pipeline(dxrPTPipeline);
		_ gLoad Pipeline(mortonIndexingPipeline);
		_ gLoad Pipeline(sortingPipeline);
		_ gLoad Pipeline(buildingOctree);
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

	void CreatingAssets(gObj<DXRManager> manager) {

		// Photons aabbs used in bottom level structure building and updates
		PhotonsAABBs = _ gCreate RWAccelerationDatastructureBuffer<D3D12_RAYTRACING_AABB>(PHOTON_DIMENSION * PHOTON_DIMENSION / BOXED_PHOTONS);

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
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		dxrPTPipeline->_Program->RadiusFactors = _ gCreate RWStructuredBuffer<float>(PHOTON_DIMENSION * PHOTON_DIMENSION);
#pragma endregion

#pragma region Morton indexing and initialization
		mortonIndexingPipeline->Photons = dxrPTPipeline->_Program->Photons;
		mortonIndexingPipeline->Indices = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		mortonIndexingPipeline->Permutation = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);
#pragma endregion

#pragma region Sorting with Bitonic
		sortingPipeline->Photons = dxrPTPipeline->_Program->Photons;
		sortingPipeline->Indices = mortonIndexingPipeline->Indices;
		sortingPipeline->Permutation = mortonIndexingPipeline->Permutation;
#pragma endregion

#pragma region Building Octree

		buildingOctree->Indices = mortonIndexingPipeline->Indices;
		buildingOctree->Permutation = mortonIndexingPipeline->Permutation;
		buildingOctree->LOD = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		buildingOctree->Skip = _ gCreate RWStructuredBuffer<int>(PHOTON_DIMENSION * PHOTON_DIMENSION);

#pragma endregion

#pragma region DXR Photon gathering Pipeline Objects
		dxrRTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrRTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrRTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrRTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrRTPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrRTPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrRTPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrRTPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		dxrRTPipeline->_Program->LightPositions = gBufferFromLight->pipeline->GBuffer_P;

		// CBs will be updated every frame
		dxrRTPipeline->_Program->CameraCB = computeDirectLighting->ViewTransform;
		dxrRTPipeline->_Program->LightTransforms = computeDirectLighting->LightTransforms;

		// Reused CBs from dxrPTPipeline
		dxrRTPipeline->_Program->LightingCB = dxrPTPipeline->_Program->LightingCB;

		dxrRTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		dxrRTPipeline->_Program->Accum = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		dxrRTPipeline->_Program->DirectLighting = DirectLighting;

		// Bind now Photon map as SRVs
		dxrRTPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		dxrRTPipeline->_Program->Radii = dxrPTPipeline->_Program->RadiusFactors;

		dxrRTPipeline->_Program->Morton = mortonIndexingPipeline->Indices;
		dxrRTPipeline->_Program->Permutation = mortonIndexingPipeline->Permutation;
		dxrRTPipeline->_Program->LOD = buildingOctree->LOD;
		dxrRTPipeline->_Program->Skip = buildingOctree->Skip;
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

		perform(Photontracing);

		perform(ConstructPhotonMap);

		perform(Raytracing);
	}

	int len; int dif;

	void Photontracing(gObj<DXRManager> manager) {

		auto ptRTProgram = dxrPTPipeline->_Program;

		static int FrameIndex = 0;

		if (CameraIsDirty || LightSourceIsDirty)
			FrameIndex = 0;

		ptRTProgram->ProgressivePass = FrameIndex;

		FrameIndex++;

		manager gCopy ValueData(ptRTProgram->CameraCB, lightView.getInverse());

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
	}

	void ConstructPhotonMap(gObj<DXRManager> manager) {
		auto computeManager = manager.Dynamic_Cast<ComputeManager>();

		// Indexing
		computeManager gSet Pipeline(mortonIndexingPipeline);
		computeManager gDispatch Threads(PHOTON_DIMENSION * PHOTON_DIMENSION / CS_1D_GROUPSIZE);

		// Sorting
		computeManager gSet Pipeline(sortingPipeline);

		int n = PHOTON_DIMENSION * PHOTON_DIMENSION;
		for (len = 2; len <= n; len <<= 1)
			for (dif = len >> 1; dif > 0; dif >>= 1)
			{
				sortingPipeline->Stage = BitonicStage{ len, dif };
				// Bitonic sort wave
				computeManager gDispatch Threads(PHOTON_DIMENSION * PHOTON_DIMENSION / (2 * CS_1D_GROUPSIZE));
			}

		computeManager gSet Pipeline(buildingOctree);
		for (int i = 0; i <= PHOTONS_LOD * 2; i++) {
			buildingOctree->BlockLOD = i;
			computeManager gDispatch Threads((int)ceil((PHOTON_DIMENSION * PHOTON_DIMENSION >> i) / (double)(CS_1D_GROUPSIZE)));
		}
	}

	void Raytracing(gObj<DXRManager> manager) {

		auto rtProgram = dxrRTPipeline->_Program;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			manager gClear UAV(rtProgram->Accum, 0u);
			manager gClear UAV(rtProgram->Output, 0u);
		}

		rtProgram->ProgressiveCB = dxrPTPipeline->_Program->ProgressivePass;

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