#pragma once

class DXRBasicScenePhotontracer : public Technique, public IHasScene, public IHasCamera, public IHasLight {
public:

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
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\DXR_Photontracer\\DXR_PhotonMapping_RT.cso"));

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
			gObj<Texture2D> *Textures;
			int TextureCount;
			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			gObj<Buffer> SpaceInfoCB;

			gObj<Texture2D> ScreenOutput;

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
				UAV(0, ScreenOutput);

				UAV(1, HeadBuffer);
				UAV(2, Malloc);
				UAV(3, Photons);
				UAV(4, NextBuffer);

				ADS(0, Scene);
				SRV(1, Vertices);
				SRV(2, Materials);
				SRV_Array(3, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, SpaceInfoCB);
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

protected:
	gObj<DXR_Pipeline> Pipeline;

	void Startup() {
		_ gLoad Pipeline(Pipeline);

		perform(LoadingSceneAssets);

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

	#define RESOLUTION 128
	#define DISPATCH_RAYS_DIMENSION 1024
	#define MAX_NUMBER_OF_PHOTONS (DISPATCH_RAYS_DIMENSION*DISPATCH_RAYS_DIMENSION*3)

	void LoadingSceneAssets(gObj<CopyingManager> manager) {
		// loading scene textures
		Pipeline->_Program->TextureCount = Scene->Textures().size();
		Pipeline->_Program->Textures = new gObj<Texture2D>[Pipeline->_Program->TextureCount];
		for (int i = 0; i < Scene->Textures().size(); i++)
			manager gLoad FromData(Pipeline->_Program->Textures[i], Scene->Textures()[i]);

		// SRV to bind to the GPU for vertices accessing
		// Must be used VB as well, but I got an error,
		// This is a work around
		Pipeline->_Program->Vertices = _ gCreate StructuredBuffer<SCENE_VERTEX>(Scene->VerticesCount());
		manager gCopy PtrData(Pipeline->_Program->Vertices, Scene->Vertices());

		Pipeline->_Program->Materials = _ gCreate StructuredBuffer<SCENE_MATERIAL>(Scene->MaterialsCount());
		manager gCopy PtrData(Pipeline->_Program->Materials, &Scene->Materials().first());

		// CBs will be updated every frame
		Pipeline->_Program->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		Pipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		Pipeline->_Program->SpaceInfoCB = _ gCreate ConstantBuffer <SpaceInfo>();

		Pipeline->_Program->ScreenOutput = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		Pipeline->_Program->Malloc = _ gCreate RWStructuredBuffer<int>(4);
		Pipeline->_Program->HeadBuffer = _ gCreate RWStructuredBuffer<int>(RESOLUTION*RESOLUTION*RESOLUTION);
		Pipeline->_Program->Photons = _ gCreate RWStructuredBuffer<Photon>(MAX_NUMBER_OF_PHOTONS);
		Pipeline->_Program->NextBuffer = _ gCreate RWStructuredBuffer<int>(MAX_NUMBER_OF_PHOTONS);
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
		Pipeline->_Program->Scene = instances gCreate BakedScene();
	}

	void Frame() {
		perform(Photontracing);

		wait_for(signal(flush_all_to_gpu));

		perform(Raytracing);
	}

	void Photontracing(gObj<DXRManager> manager) {

		auto rtProgram = Pipeline->_Program;

		// Update lighting needed for photon tracing
		manager gCopy ValueData(rtProgram->LightingCB, Lighting{
			Light->Position, 0,
			Light->Intensity, 0
			});

		float3 MinimumPosition{ -2, -2, -2 };
		float3 BoxSize{ 4, 4, 4 };
		int3 resolution{ RESOLUTION, RESOLUTION, RESOLUTION };

		// Update SpaceInfo
		manager gCopy ValueData(rtProgram->SpaceInfoCB, SpaceInfo{
			MinimumPosition, 0, // Minimum Position
			BoxSize, 0, // Box Size,
			BoxSize / resolution, 0, // Cell Size
			resolution, 0
			});

		// Set DXR Pipeline
		manager gSet Pipeline(Pipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);
		
		manager gClear UAV(rtProgram->Malloc, 0U); // reset allocation pointer
		
		manager gClear UAV(rtProgram->HeadBuffer, (unsigned int)-1); // reset head buffer to null for every list

		int startTriangle;

#pragma region Photon Trace Stage
		
		// Set Miss in slot 0
		manager gSet Miss(Pipeline->PhotonMiss, 0);

		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			manager gSet HitGroup(Pipeline->PhotonMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(Pipeline->PTMainRays);

		// Dispatch rays for 1 000 000 photons
		manager gDispatch Rays(DISPATCH_RAYS_DIMENSION, DISPATCH_RAYS_DIMENSION);

#pragma endregion
	}

	void Raytracing(gObj<DXRManager> manager) {

		auto rtProgram = Pipeline->_Program;

		// Update camera
		// Required during ray-trace stage
		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
		float4x4 projToWorld = mul(view, proj).getInverse();
		manager gCopy ValueData(rtProgram->CameraCB, projToWorld);

		//float3 MinimumPosition{ -2, -2, -2 };
		//float3 BoxSize{ 4, 4, 4 };
		//int3 resolution{ RESOLUTION, RESOLUTION, RESOLUTION };

		//// Update SpaceInfo
		//manager gCopy ValueData(rtProgram->SpaceInfoCB, SpaceInfo{
		//	MinimumPosition, 0, // Minimum Position
		//	BoxSize, 0, // Box Size,
		//	BoxSize / resolution, 0, // Cell Size
		//	resolution, 0
		//	});

		// Set DXR Pipeline
		manager gSet Pipeline(Pipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Raytrace Stage

		// Set Miss in slot 0
		manager gSet Miss(Pipeline->EnvironmentMap, 0);

		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			manager gSet HitGroup(Pipeline->RTMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(Pipeline->RTMainRays);

		// Dispatch primary rays
		manager gDispatch Rays(render_target->Width, render_target->Height);

		// Copy DXR output texture to the render target
		manager gCopy All(render_target, rtProgram->ScreenOutput);

#pragma endregion
	}

};