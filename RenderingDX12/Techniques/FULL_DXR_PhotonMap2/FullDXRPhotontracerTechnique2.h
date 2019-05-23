#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"

struct FullDXRPhotonTracer2 : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

#define DISPATCH_RAYS_DIMENSION 256

#define MAX_NUMBER_OF_PHOTONS (DISPATCH_RAYS_DIMENSION*DISPATCH_RAYS_DIMENSION*3)

	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;
	// GBuffer process used to build GBuffer data from light
	gObj<GBufferConstruction> gBufferFromLight;
	// GBuffer process used to build GBuffer data from viewer
	gObj<GBufferConstruction> gBufferFromViewer;

	// DXR pipeline for photon tracing stage
	struct DXR_PT_Pipeline : public RTPipelineManager {
		// Inherited via RTPipelineManager

		gObj<RayGenerationHandle> PTMainRays;
		gObj<MissHandle> PhotonMiss;
		gObj<ClosestHitHandle> PhotonScattering;
		gObj<HitGroupHandle> PhotonMaterial;

		class DXR_PT_IL : public DXIL_Library<DXR_PT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\FULL_DXR_PhotonMap2\\FullDXRPhotonTracing2_RT.cso"));

				_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
				_ gLoad Shader(Context()->PhotonMiss, L"PhotonMiss");
				_ gLoad Shader(Context()->PhotonScattering, L"PhotonScattering");
			}
		};
		gObj<DXR_PT_IL> _Library;

		struct DXR_PT_Program : public RTProgram<DXR_PT_Pipeline> {
			void Setup() {
				_ gSet Payload(16);
				_ gSet StackSize(3);
				_ gLoad Shader(Context()->PTMainRays);
				_ gLoad Shader(Context()->PhotonMiss);
				_ gCreate HitGroup(Context()->PhotonMaterial, Context()->PhotonScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;

			// GBuffer Information
			gObj<Texture2D> Positions;
			gObj<Texture2D> Normals;
			gObj<Texture2D> Coordinates;
			gObj<Texture2D> MaterialIndices;

			gObj<Texture2D> *Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;

			// Photon map binding objects
			gObj<Buffer> PhotonAABBs; // Buffer with generated photon's aabbs for BVH photon map building
			gObj<Buffer> Photons; // Buffer with all Photon attributes
			gObj<Texture2D> Malloc; // buffer used to allocate memory using InterlockedAdd. one single element needed

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
				int2 padding;
			} CurrentObjectInfo;

			void Globals() {
				UAV(0, PhotonAABBs);
				UAV(1, Photons);
				UAV(2, Malloc);

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
		gObj<DXR_PT_Program> _Program;

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
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\FULL_DXR_PhotonMap2\\FullDXRRTWithDeferedGathering2_RT.cso"));

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
				_ gSet Payload(4 * (3 + 1 + 3 + 3)); // 3- Normal, 1- SpecularSharpness, 3- OutDiffAcc, 3- OutSpecAcc
				_ gSet StackSize(3);
				_ gSet MaxHitGroupIndex(MAX_NUMBER_OF_PHOTONS + 1000); // photons + number of geometries
				_ gLoad Shader(Context()->RTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gLoad Shader(Context()->PhotonGatheringMiss);
				_ gCreate HitGroup(Context()->RTMaterial, Context()->RTScattering, nullptr, nullptr);
				_ gCreate HitGroup(Context()->PhotonGatheringMaterial, nullptr, Context()->PhotonGatheringAnyHit, Context()->PhotonGatheringIntersection);
			}

			gObj<SceneOnGPU> Scene;
			gObj<Buffer> Photons;
			gObj<Texture2D> PhotonCount;

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

			gObj<Texture2D> Output;


			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
				int2 padding;
			} CurrentObjectInfo;

			void Globals() {
				UAV(0, Output);

				ADS(0, Scene); // Scene with both Photon AABBs and Scene geometries

				SRV(1, Photons);
				SRV(2, PhotonCount);

				SRV(3, Vertices);
				SRV(4, Materials);

				SRV(5, Positions);
				SRV(6, Normals);
				SRV(7, Coordinates);
				SRV(8, MaterialIndices);

				SRV(9, LightPositions);

				SRV_Array(10, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, LightTransforms);
			}

			void HitGroup_Locals() {
				CBV(3, CurrentObjectInfo);
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
	gObj<DXR_RT_Pipeline> dxrRTPipeline;

	gObj<GeometriesOnGPU> PhotonAABBsOnTheGPU;
	// AABBs buffer for photon map
	gObj<Buffer> PhotonsAABBs;
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
		gBufferFromLight = new GBufferConstruction(DISPATCH_RAYS_DIMENSION, DISPATCH_RAYS_DIMENSION);
		gBufferFromLight->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromLight);

		// Load and setup gbuffer construction process from viewer
		gBufferFromViewer = new GBufferConstruction(render_target->Width, render_target->Height);
		gBufferFromViewer->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromViewer);

		wait_for(signal(flush_all_to_gpu));

		_ gLoad Pipeline(dxrPTPipeline);
		_ gLoad Pipeline(dxrRTPipeline);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		flush_all_to_gpu;

		perform(CreateSceneOnGPU);
	}

	struct Photon {
		float3 Position;
		float3 Direction;
		float3 Intensity;
		float Radius;
	};


	void CreatingAssets(gObj<CopyingManager> manager) {
		// Photons aabbs used in bottom level structure building and updates
		PhotonsAABBs = _ gCreate RWAccelerationDatastructureBuffer<D3D12_RAYTRACING_AABB>(MAX_NUMBER_OF_PHOTONS);
		D3D12_RAYTRACING_AABB* aabbs = new D3D12_RAYTRACING_AABB[MAX_NUMBER_OF_PHOTONS];
		for (int i = 0; i < MAX_NUMBER_OF_PHOTONS; i++)
		{
			float x = 4 * (rand() % 1000) / 1000.0f - 2;
			float y = 4 * (rand() % 1000) / 1000.0f - 2;
			float z = 4 * (rand() % 1000) / 1000.0f - 2;
			aabbs[i] = { x,y,z, x + 0.005f, y + 0.005f ,z + 0.005f };
		}
		manager gCopy PtrData(PhotonsAABBs, aabbs);
		delete aabbs;
		//PhotonsAABBs = _ gCreate GenericBuffer<D3D12_RAYTRACING_AABB>(D3D12_RESOURCE_STATE_GENERIC_READ, MAX_NUMBER_OF_PHOTONS, CPU_WRITE_GPU_READ);

#pragma region DXR Photon trace Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;

		// CBs will be updated every frame
		dxrPTPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPTPipeline->_Program->PhotonAABBs = PhotonsAABBs;
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(MAX_NUMBER_OF_PHOTONS);
		dxrPTPipeline->_Program->Malloc = _ gCreate DrawableTexture2D<unsigned int>(1, 1);
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

		dxrRTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		// Bind now Photon map as SRVs
		dxrRTPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		// Photon counter during photon trace is used to decide wich photons are 
		dxrRTPipeline->_Program->PhotonCount = dxrPTPipeline->_Program->Malloc;
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
		gObj<GeometriesOnGPU> sceneGeometriesOnGPU = sceneGeometriesBuilder gCreate BakedGeometry();

		auto photonMapBuilder = manager gCreate ProceduralGeometries();
		photonMapBuilder gSet AABBs(PhotonsAABBs);
		photonMapBuilder gLoad Geometry(0, MAX_NUMBER_OF_PHOTONS);
		PhotonAABBsOnTheGPU = photonMapBuilder gCreate BakedGeometry(true, true);

		auto sceneInstances = manager gCreate Instances();
		sceneInstances gLoad Instance(PhotonAABBsOnTheGPU, 2, 0);
		// Mask 1 to represent triangle (scene) data
		// contribution is used to offset hit group to tresspass all photons
		sceneInstances gLoad Instance(sceneGeometriesOnGPU, 1, MAX_NUMBER_OF_PHOTONS);

		dxrPTPipeline->_Program->Scene = dxrRTPipeline->_Program->Scene = sceneInstances gCreate BakedScene(true, true);
	}

	float4x4 view, proj;
	float4x4 lightView, lightProj;

	void Frame() {
#pragma region Construct GBuffer from viewer
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		gBufferFromViewer->ViewMatrix = view;
		gBufferFromViewer->ProjectionMatrix = proj;
		ExecuteFrame(gBufferFromViewer);
#pragma endregion

#pragma region Construct GBuffer from light
		lightView = LookAtLH(this->Light->Position, this->Light->Position + float3(0, -1, 0), float3(0, 0, 1));
		lightProj = PerspectiveFovLH(PI / 2, 1, 0.001f, 10);
		gBufferFromLight->ViewMatrix = lightView;
		gBufferFromLight->ProjectionMatrix = lightProj;
		ExecuteFrame(gBufferFromLight);
#pragma endregion

		perform(Photontracing);

		flush_all_to_gpu; // Grant PhotonAABBs was fully updated

		perform(BuildPhotonMap);

		flush_all_to_gpu; // Grant PhotonMap was fully updated

		perform(Raytracing);
	}

	void Photontracing(gObj<DXRManager> manager) {

		auto ptRTProgram = dxrPTPipeline->_Program;

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

		manager gClear UAV(ptRTProgram->Malloc, 0U); // reset allocation pointer

		int startTriangle;

#pragma region Photon Trace Stage

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

			manager gSet HitGroup(dxrPTPipeline->PhotonMaterial, MAX_NUMBER_OF_PHOTONS + i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

		// Dispatch rays for more than 2^22 photons
		manager gDispatch Rays(DISPATCH_RAYS_DIMENSION, DISPATCH_RAYS_DIMENSION);

#pragma endregion
	}

	void BuildPhotonMap(gObj<DXRManager> manager) {
#pragma region Update Photon Map BVHs in next frames
		auto geomUpdater = manager gCreate ProceduralGeometries(PhotonAABBsOnTheGPU);
		geomUpdater->PrepareBuffer(PhotonsAABBs);
		geomUpdater gSet AABBs(PhotonsAABBs);
		geomUpdater gLoad Geometry(0, MAX_NUMBER_OF_PHOTONS);
		geomUpdater gCreate UpdatedGeometry();
#pragma endregion
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

		// Set DXR Pipeline
		manager gSet Pipeline(dxrRTPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Raytrace Stage

		// Set Environment Miss in slot 0
		manager gSet Miss(dxrRTPipeline->EnvironmentMap, 0);
		// Set PhotonGatheringMiss in slot 1
		manager gSet Miss(dxrRTPipeline->PhotonGatheringMiss, 1);

		// Set PhotonGatheringMaterial for each photon in ST
		for (int i = 0; i < MAX_NUMBER_OF_PHOTONS; i++)
		{
			rtProgram->CurrentObjectInfo = {
				i, i
			};
			manager gSet HitGroup(dxrRTPipeline->PhotonGatheringMaterial, i);
		}
		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			manager gSet HitGroup(dxrRTPipeline->RTMaterial, MAX_NUMBER_OF_PHOTONS + i);

			startTriangle += sceneObject.vertexesCount / 3;
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