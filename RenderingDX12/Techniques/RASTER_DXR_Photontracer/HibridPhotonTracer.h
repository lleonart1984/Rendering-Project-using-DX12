#pragma once

#include "../DeferredShading/GBufferConstruction.h"

class HybridPhotonTracerTechnique : public Technique, public IHasScene, public IHasLight, public IHasCamera {
public:

#define RESOLUTION 128
#define DISPATCH_RAYS_DIMENSION 1024
#define MAX_NUMBER_OF_PHOTONS (DISPATCH_RAYS_DIMENSION*DISPATCH_RAYS_DIMENSION*3)

	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;
	// GBuffer process used to build GBuffer data from light
	gObj<GBufferConstruction> gBufferFromLight;
	// GBuffer process used to build GBuffer data from viewer
	gObj<GBufferConstruction> gBufferFromViewer;

	struct DeferredHybridShadingPipeline : ShowTexturePipeline {
	
		gObj<Texture2D> Positions;
		gObj<Texture2D> PositionsFromLight;
		gObj<Texture2D> DirectLighting;
		gObj<Texture2D> IndirectLighting;

		gObj<Buffer> FromViewToLightSpaceCB;
		
		void Setup() {
			ShowTexturePipeline::Setup();
			_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\RASTER_DXR_Photontracer\\DeferredHybridShading_PS.cso"));
		}

		void Globals() {
			RTV(0, RenderTarget);

			CBV(0, FromViewToLightSpaceCB, ShaderType_Pixel);
			
			SRV(0, Positions, ShaderType_Pixel);
			SRV(1, PositionsFromLight, ShaderType_Pixel);
			SRV(2, DirectLighting, ShaderType_Pixel);
			SRV(3, IndirectLighting, ShaderType_Pixel);
			
			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Pixel);
		}
	};

	struct DXR_Pipeline : public RTPipelineManager {
		// Inherited via RTPipelineManager

		gObj<RayGenerationHandle> PTMainRays;
		gObj<RayGenerationHandle> RTMainRays;
		gObj<MissHandle> PhotonMiss;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> PhotonScattering;
		gObj<ClosestHitHandle> RTScattering;
		gObj<HitGroupHandle> PhotonMaterial;
		gObj<HitGroupHandle> RTMaterial;

		class DXR_Basic_IL : public DXIL_Library<DXR_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\RASTER_DXR_Photontracer\\HibridPhotonTracer_RT.cso"));

				_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
				_ gLoad Shader(Context()->RTMainRays, L"RTMainRays");
				_ gLoad Shader(Context()->PhotonMiss, L"PhotonMiss");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->PhotonScattering, L"PhotonScattering");
				_ gLoad Shader(Context()->RTScattering, L"RTScattering");
			}
		};
		gObj<DXR_Basic_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_Pipeline> {
			void Setup() {
				_ gSet Payload(16);
				_ gSet StackSize(6);
				_ gLoad Shader(Context()->PTMainRays);
				_ gLoad Shader(Context()->RTMainRays);
				_ gLoad Shader(Context()->PhotonMiss);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->PhotonMaterial, Context()->PhotonScattering, nullptr, nullptr);
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
			gObj<Texture2D> LightPositions;

			gObj<Texture2D> *Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			gObj<Buffer> SpaceInfoCB;
			gObj<Buffer> LightTransforms;

			gObj<Texture2D> Output;
			
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
				UAV(0, Output);

				UAV(1, HeadBuffer);
				UAV(2, Malloc);
				UAV(3, Photons);
				UAV(4, NextBuffer);

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
	gObj<DXR_Pipeline> dxrPipeline;
	gObj<DeferredHybridShadingPipeline> deferredPipeline;

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

		// Load the pipeline for drawing deferred lighting
		_ gLoad Pipeline(deferredPipeline);

		_ gLoad Pipeline(dxrPipeline);

		deferredPipeline->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		deferredPipeline->PositionsFromLight = gBufferFromLight->pipeline->GBuffer_P;
		deferredPipeline->FromViewToLightSpaceCB = _ gCreate ConstantBuffer<Globals>();

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

#pragma region DXR Pipeline Objects
		dxrPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPipeline->_Program->Vertices = sceneLoader->VertexBuffer;

		// CBs will be updated every frame
		dxrPipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		dxrPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPipeline->_Program->SpaceInfoCB = _ gCreate ConstantBuffer <SpaceInfo>();
		dxrPipeline->_Program->LightTransforms = _ gCreate ConstantBuffer <Globals>();

		dxrPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		dxrPipeline->_Program->Malloc = _ gCreate RWStructuredBuffer<int>(4);
		dxrPipeline->_Program->HeadBuffer = _ gCreate RWStructuredBuffer<int>(RESOLUTION*RESOLUTION*RESOLUTION);
		dxrPipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(MAX_NUMBER_OF_PHOTONS);
		dxrPipeline->_Program->NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_PHOTONS);
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
		dxrPipeline->_Program->Scene = instances gCreate BakedScene();
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
		lightProj = PerspectiveFovLH(PI / 2, 1, 0.01f, 10);
		gBufferFromLight->ViewMatrix = lightView;
		gBufferFromLight->ProjectionMatrix = lightProj;
		ExecuteFrame(gBufferFromLight);
#pragma endregion

		//wait_for(signal(flush_all_to_gpu));

		perform(Photontracing);

		wait_for(signal(flush_all_to_gpu));

		perform(Raytracing);

	}

	void Photontracing(gObj<DXRManager> manager) {

		auto rtProgram = dxrPipeline->_Program;

		// Update lighting needed for photon tracing
		manager gCopy ValueData(rtProgram->LightingCB, Lighting{
			Light->Position, 0,
			Light->Intensity, 0
			});

		manager gCopy ValueData(rtProgram->CameraCB, lightView.getInverse());

		float3 MinimumPosition{ -1, -1, -1 };
		float3 BoxSize{ 2, 2, 2 };
		int3 resolution{ RESOLUTION, RESOLUTION, RESOLUTION };

		// Update SpaceInfo
		manager gCopy ValueData(rtProgram->SpaceInfoCB, SpaceInfo{
			MinimumPosition, 0, // Minimum Position
			BoxSize, 0, // Box Size,
			BoxSize / resolution, 0, // Cell Size
			resolution, 0
			});

		dxrPipeline->_Program->Positions = gBufferFromLight->pipeline->GBuffer_P;
		dxrPipeline->_Program->Normals = gBufferFromLight->pipeline->GBuffer_N;
		dxrPipeline->_Program->Coordinates = gBufferFromLight->pipeline->GBuffer_C;
		dxrPipeline->_Program->MaterialIndices = gBufferFromLight->pipeline->GBuffer_M;

		// Set DXR Pipeline
		manager gSet Pipeline(dxrPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		manager gClear UAV(rtProgram->Malloc, 0U); // reset allocation pointer

		manager gClear UAV(rtProgram->HeadBuffer, (unsigned int)-1); // reset head buffer to null for every list

		int startTriangle;

#pragma region Photon Trace Stage

		// Set Miss in slot 0
		manager gSet Miss(dxrPipeline->PhotonMiss, 0);

		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			manager gSet HitGroup(dxrPipeline->PhotonMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPipeline->PTMainRays);

		// Dispatch rays for 1 000 000 photons
		manager gDispatch Rays(DISPATCH_RAYS_DIMENSION, DISPATCH_RAYS_DIMENSION);

#pragma endregion
	}

	void Raytracing(gObj<DXRManager> manager) {

		auto rtProgram = dxrPipeline->_Program;

		// Update camera
		// Required during ray-trace stage
		manager gCopy ValueData(rtProgram->CameraCB, view.getInverse());

		float3 MinimumPosition{ -1, -1, -1 };
		float3 BoxSize{ 2, 2, 2 };
		int3 resolution{ RESOLUTION, RESOLUTION, RESOLUTION };

		// Update SpaceInfo
		manager gCopy ValueData(rtProgram->SpaceInfoCB, SpaceInfo{
			MinimumPosition, 0, // Minimum Position
			BoxSize, 0, // Box Size,
			BoxSize / resolution, 0, // Cell Size
			resolution, 0
			});

		manager gCopy ValueData(rtProgram->LightTransforms, Globals{
				lightProj,
				lightView
			});

		dxrPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		dxrPipeline->_Program->LightPositions = gBufferFromLight->pipeline->GBuffer_P;

		// Set DXR Pipeline
		manager gSet Pipeline(dxrPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Raytrace Stage

		// Set Miss in slot 0
		manager gSet Miss(dxrPipeline->EnvironmentMap, 0);

		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			manager gSet HitGroup(dxrPipeline->RTMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPipeline->RTMainRays);

		// Dispatch primary rays
		manager gDispatch Rays(render_target->Width, render_target->Height);

		// Copy DXR output texture to the render target
		manager gCopy All(render_target, rtProgram->Output);
#pragma endregion
	}

};