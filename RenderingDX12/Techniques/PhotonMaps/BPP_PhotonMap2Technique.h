#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"
#include "../CommonRT/DirectLightingTechnique.h"

struct BPP_PhotonMap2Technique : public DirectLightingTechnique {
public:

#include "DXR_PhotonTracing_Pipeline.h"

	// ComputeShader pipeline for AABB construction and Radii population
	struct ConstructPhotonMapPipeline : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PhotonConstruction_CS.cso"));
		}

		gObj<Buffer> Photons;
		gObj<Buffer> AABBs;
		gObj<Buffer> Radii;
		gObj<Buffer> RadiusFactors;

		void Globals() {
			UAV(0, AABBs, ShaderType_Any);
			UAV(1, Radii, ShaderType_Any);

			SRV(0, Photons, ShaderType_Any);
			SRV(1, RadiusFactors, ShaderType_Any);
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

			gObj<SceneOnGPU> Scene;
			gObj<SceneOnGPU> PhotonMap;
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
			int2 ProgressiveCB;
			gObj<Buffer> LightTransforms;

			gObj<Texture2D> Output;
			gObj<Texture2D> Accum;
			gObj<Texture2D> DirectLighting;

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

				ADS(10, PhotonMap);
				SRV(11, Radii);

				SRV_Array(12, Textures, TextureCount);

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
	gObj<ConstructPhotonMapPipeline> constructionPipeline;
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
		_ gLoad Pipeline(constructionPipeline);
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

	void CreatingAssets(gObj<CopyingManager> manager) {
		// Photons aabbs used in bottom level structure building and updates
		PhotonsAABBs = _ gCreate RWAccelerationDatastructureBuffer<D3D12_RAYTRACING_AABB>(PHOTON_DIMENSION * PHOTON_DIMENSION);

#pragma region DXR Photon trace Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrPTPipeline->_Program->Positions = gBufferFromLight->pipeline->GBuffer_P;
		dxrPTPipeline->_Program->Normals = gBufferFromLight->pipeline->GBuffer_N;
		dxrPTPipeline->_Program->Coordinates = gBufferFromLight->pipeline->GBuffer_C;
		dxrPTPipeline->_Program->MaterialIndices = gBufferFromLight->pipeline->GBuffer_M;


		// CBs will be updated every frame
		dxrPTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->LightingCB = computeDirectLighting->Lighting;
		//dxrPTPipeline->_Program->ProgressivePass = _ gCreate ConstantBuffer<int>();
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		dxrPTPipeline->_Program->RadiusFactors = _ gCreate RWStructuredBuffer<float>(PHOTON_DIMENSION * PHOTON_DIMENSION);
#pragma endregion

#pragma region DXR Pipeline for PM construction using AABBs
		constructionPipeline->AABBs = PhotonsAABBs;
		constructionPipeline->Radii = _ gCreate RWStructuredBuffer<float>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		constructionPipeline->Photons = dxrPTPipeline->_Program->Photons;
		constructionPipeline->RadiusFactors = dxrPTPipeline->_Program->RadiusFactors;
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
		dxrRTPipeline->_Program->Radii = constructionPipeline->Radii;
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

	void CreatePhotonMapOnGPU(gObj<DXRManager> manager) {
		// Building top-level ADS for PhotonMap
		auto sceneInstances = manager gCreate Instances();

		auto photonMapBuilder = manager gCreate ProceduralGeometries();
		photonMapBuilder gSet AABBs(PhotonsAABBs);
		photonMapBuilder gLoad Geometry(0, PHOTON_DIMENSION * PHOTON_DIMENSION);
		PhotonsAABBsOnTheGPU = photonMapBuilder gCreate BakedGeometry(false, true);
		sceneInstances gLoad Instance(PhotonsAABBsOnTheGPU, 2, 0, 0);

		dxrRTPipeline->_Program->PhotonMap = sceneInstances gCreate BakedScene(true, true);
	}

	void UpdatePhotonMap(gObj<DXRManager> manager) {

		auto sceneInstances = manager gCreate Instances(dxrRTPipeline->_Program->PhotonMap);

		auto photonMapBuilder = manager gCreate ProceduralGeometries(PhotonsAABBsOnTheGPU);
		photonMapBuilder gSet AABBs(PhotonsAABBs);
		photonMapBuilder gLoad Geometry(0, PHOTON_DIMENSION * PHOTON_DIMENSION);
		PhotonsAABBsOnTheGPU = photonMapBuilder gCreate RebuiltGeometry(false, true);
		sceneInstances gLoad Instance(PhotonsAABBsOnTheGPU, 2, 0, 0);
		dxrRTPipeline->_Program->PhotonMap = sceneInstances gCreate UpdatedScene();
	}

	void Frame() {

		DirectLightingTechnique::Frame();

		perform(Photontracing);

		perform(ConstructPhotonMap);

		static bool firstTime = true;
		if (firstTime) {
			perform(CreatePhotonMapOnGPU);
			firstTime = false;
		}
		else
			perform(UpdatePhotonMap);

		perform(Raytracing);
	}

	void Photontracing(gObj<DXRManager> manager) {

		auto ptRTProgram = dxrPTPipeline->_Program;

		static int FrameIndex = 0;

		if (CameraIsDirty || LightSourceIsDirty)
			FrameIndex = 0;

		ptRTProgram->ProgressivePass = FrameIndex;

		FrameIndex++;

		if (LightSourceIsDirty) {
			manager gCopy ValueData(ptRTProgram->CameraCB, lightView.getInverse());
		}

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

	void ConstructPhotonMap(gObj<DXRManager> manager) {
		auto computeManager = manager.Dynamic_Cast<ComputeManager>();

		computeManager gSet Pipeline(constructionPipeline);
		computeManager gDispatch Threads(PHOTON_DIMENSION * PHOTON_DIMENSION / CS_1D_GROUPSIZE);
	}

	void Raytracing(gObj<DXRManager> manager) {

		auto rtProgram = dxrRTPipeline->_Program;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			manager gClear UAV(rtProgram->Output, 0u);
			manager gClear UAV(rtProgram->Accum, 0u);
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