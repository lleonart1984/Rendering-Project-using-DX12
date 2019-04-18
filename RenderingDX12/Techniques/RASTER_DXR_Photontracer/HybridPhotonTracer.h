#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"

struct HybridPhotonTracer : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

	~HybridPhotonTracer() {
	}
#define RESOLUTION 128
#define DISPATCH_RAYS_DIMENSION 2048
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
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\RASTER_DXR_Photontracer\\HPhotonTracer_RT.cso"));

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
			gObj<Buffer> SpaceInfoCB;

			// Photon map binding objects
			gObj<Buffer> HeadBuffer; // head ptrs of linked lists
			gObj<Buffer> Malloc; // buffer used to allocate memory using InterlockedAdd. one single element needed
			gObj<Buffer> Photons; // Photon map in a lineal buffer
			gObj<Buffer> NextBuffer; // Reference to next element in each linked list node

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
			} CurrentObjectInfo;

			void Globals() {
				UAV(0, HeadBuffer);
				UAV(1, Malloc);
				UAV(2, Photons);
				UAV(3, NextBuffer);

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
				CBV(2, SpaceInfoCB);
			}

			void HitGroup_Locals() {
				CBV(3, CurrentObjectInfo);
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

		class DXR_RT_IL : public DXIL_Library<DXR_RT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\RASTER_DXR_Photontracer\\HPhotonGrathering_RT.cso"));

				_ gLoad Shader(Context()->RTMainRays, L"RTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->RTScattering, L"RTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_RT_Pipeline> {
			void Setup() {
				_ gSet Payload(16);
				_ gSet StackSize(3);
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
			// GBuffer from light for visibility test during direct lighting
			gObj<Texture2D> LightPositions;

			// Photon map binding objects (now as readonly-resources)
			gObj<Buffer> HeadBuffer; // head ptrs of linked lists
			gObj<Buffer> Photons; // Photon map in a lineal buffer
			gObj<Buffer> NextBuffer; // Reference to next element in each linked list node

			gObj<Texture2D> *Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			gObj<Buffer> SpaceInfoCB;
			gObj<Buffer> LightTransforms;

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
				
				SRV(8, HeadBuffer);
				SRV(9, Photons);
				SRV(10, NextBuffer);

				SRV_Array(11, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, SpaceInfoCB);
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

		perform(CreateSceneOnGPU);
	}

	struct SpaceInfo {
		float3 MinimumPosition; float pad0;
		float3 BoxSize; float pad1;
		float3 CellSize; float pad2;
		int3 Resolution; float pad3;
	};

	struct Photon {
		float3 Position;
		float3 Direction;
		float3 Intensity;
	};


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
		dxrPTPipeline->_Program->SpaceInfoCB = _ gCreate ConstantBuffer <SpaceInfo>();
		dxrPTPipeline->_Program->Malloc = _ gCreate RWStructuredBuffer<int>(4);
		dxrPTPipeline->_Program->HeadBuffer = _ gCreate RWStructuredBuffer<int>(RESOLUTION*RESOLUTION*RESOLUTION);
		dxrPTPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(MAX_NUMBER_OF_PHOTONS);
		dxrPTPipeline->_Program->NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_PHOTONS);
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
		dxrRTPipeline->_Program->SpaceInfoCB = dxrPTPipeline->_Program->SpaceInfoCB;

		dxrRTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		// Bind now as SRVs
		dxrRTPipeline->_Program->HeadBuffer = dxrPTPipeline->_Program->HeadBuffer;
		dxrRTPipeline->_Program->Photons = dxrPTPipeline->_Program->Photons;
		dxrRTPipeline->_Program->NextBuffer = dxrPTPipeline->_Program->NextBuffer;
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

		perform(Raytracing);

		return;

//		wait_for(signal(flush_all_to_gpu));
	}

	void Photontracing(gObj<DXRManager> manager) {

		auto ptRTProgram = dxrPTPipeline->_Program;

		// Update lighting needed for photon tracing
		manager gCopy ValueData(ptRTProgram->LightingCB, Lighting{
			Light->Position, 0,
			Light->Intensity, 0
			});

		manager gCopy ValueData(ptRTProgram->CameraCB, lightView.getInverse());

		float3 MinimumPosition{ -1, -1, -1 };
		float3 BoxSize{ 2, 2, 2 };
		int3 resolution{ RESOLUTION, RESOLUTION, RESOLUTION };

		// Update SpaceInfo
		manager gCopy ValueData(ptRTProgram->SpaceInfoCB, SpaceInfo{
			MinimumPosition, 0, // Minimum Position
			BoxSize, 0, // Box Size,
			BoxSize / resolution, 0, // Cell Size
			resolution, 0
			});

		dxrPTPipeline->_Program->Positions = gBufferFromLight->pipeline->GBuffer_P;
		dxrPTPipeline->_Program->Normals = gBufferFromLight->pipeline->GBuffer_N;
		dxrPTPipeline->_Program->Coordinates = gBufferFromLight->pipeline->GBuffer_C;
		dxrPTPipeline->_Program->MaterialIndices = gBufferFromLight->pipeline->GBuffer_M;

		// Set DXR Pipeline
		manager gSet Pipeline(dxrPTPipeline);
		// Activate program with main shaders
		manager gSet Program(ptRTProgram);

		manager gClear UAV(ptRTProgram->Malloc, 0U); // reset allocation pointer

		manager gClear UAV(ptRTProgram->HeadBuffer, (unsigned int)-1); // reset head buffer to null for every list

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

			manager gSet HitGroup(dxrPTPipeline->PhotonMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

		// Dispatch rays for 1 000 000 photons
		manager gDispatch Rays(DISPATCH_RAYS_DIMENSION, DISPATCH_RAYS_DIMENSION);

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

		// Setup a raygen shader
		manager gSet RayGeneration(dxrRTPipeline->RTMainRays);

		// Dispatch primary rays
		manager gDispatch Rays(render_target->Width, render_target->Height);

		// Copy DXR output texture to the render target
		manager gCopy All(render_target, rtProgram->Output);
#pragma endregion
	}

};