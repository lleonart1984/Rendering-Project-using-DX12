#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../CommonGI/Parameters.h"

struct AVolSTWithDFTechnique : public Technique, public IHasScene, public IHasLight, public IHasCamera, public IHasScatteringEvents, public IHasAccumulative {
public:

	~AVolSTWithDFTechnique() {
	}

	struct Voxelizer : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\GridVoxelization_CS.cso"));
		}

		gObj<Buffer> Vertices;
		gObj<Buffer> OB;
		gObj<Buffer> Transforms;

		gObj<Buffer> Grid;

		gObj<Buffer> GridInfo;

		void Globals() {
			SRV(0, Vertices, ShaderType_Any);
			SRV(1, OB, ShaderType_Any);
			SRV(2, Transforms, ShaderType_Any);

			UAV(0, Grid, ShaderType_Any);

			CBV(0, GridInfo, ShaderType_Any);
		}
	};

	struct Spreading : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\GridSpread_CS.cso"));
		}

		gObj<Buffer> GridSrc;
		gObj<Buffer> GridDst;
		gObj<Buffer> GridInfo;
		int2 LevelInfo;

		void Globals() {
			UAV(0, GridDst, ShaderType_Any);
			SRV(0, GridSrc, ShaderType_Any);
			CBV(0, GridInfo, ShaderType_Any);
		}

		void Locals() {

			CBV(1, LevelInfo, ShaderType_Any);
		}
	};

	struct GridToTexture : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\GridToText3D_CS.cso"));
		}

		gObj<Buffer> GridSrc;
		gObj<Buffer> DistanceField;
		gObj<Buffer> GridInfo;

		void Globals() {
			UAV(0, DistanceField, ShaderType_Any);
			SRV(0, GridSrc, ShaderType_Any);
			CBV(0, GridInfo, ShaderType_Any);
		}
	};



	// DXR pipeline for pathtracing stage
	struct DXR_PT_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> PTMainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> PTScattering;
		gObj<HitGroupHandle> PTMaterial;

		class DXR_RT_IL : public DXIL_Library<DXR_PT_Pipeline> {
			void Setup() {
				//_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\AVolSTWithDF2_RT.cso"));
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\NVolSTWithDF_RT.cso"));

				//_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\AVolSTWithDFAndDL_RT.cso"));
				//_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\NVolSTWithDFAndDL_RT.cso"));

				_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->PTScattering, L"PTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_PT_Pipeline> {
			void Setup() {
				//_ gSet Payload(4 * 3 * 4 + 4 + 4 + 16); // 4 float3 + int + 2 uint
				_ gSet Payload(4 * 5); // 4 float3 + int + 2 uint
				_ gSet StackSize(1); // No recursion needed!
				_ gLoad Shader(Context()->PTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->PTMaterial, Context()->PTScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;

			gObj<Texture3D> Grid;

			gObj<Texture2D>* Textures;
			int TextureCount;

			gObj<Buffer> LightingCB;
			int2 Frame;
			gObj<Buffer> ParticipatingMedia;
			gObj<Buffer> ProjToWorld;
			gObj<Buffer> GridInfo;
			int Debug;

			gObj<Texture2D> DirectLighting;
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
				SRV(1, Vertices);
				SRV(2, Materials);

				SRV(3, DirectLighting);

				SRV(4, Grid);

				SRV_Array(5, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, LightingCB);
				CBV(1, Frame);
				CBV(2, ParticipatingMedia);
				CBV(3, ProjToWorld);
				CBV(4, GridInfo);
				CBV(5, Debug);

				Static_SMP(2, Sampler::PointWithoutMipMaps());
			}

			void HitGroup_Locals() {
				CBV(6, CurrentObjectInfo);
			}
		};
		gObj<DXR_RT_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};


	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader; 
	
	gObj<Voxelizer> voxelizer;
	gObj<Spreading> spreading;
	gObj<GridToTexture> gridToTexture;
	gObj<DXR_PT_Pipeline> dxrPTPipeline;

	void Startup() {

		// Load and setup scene loading process
		sceneLoader = new RetainedSceneLoader();
		sceneLoader->SetScene(this->Scene);
		_ gLoad Subprocess(sceneLoader);

		wait_for(signal(flush_all_to_gpu));

		_ gLoad Pipeline(voxelizer);
		_ gLoad Pipeline(spreading);
		_ gLoad Pipeline(gridToTexture);
		_ gLoad Pipeline(dxrPTPipeline);
		
		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		perform(CreateSceneOnGPU);
	}

	int gridSize = 512;
	struct GridInfo {
		int Size;
		float3 Min;
		float3 Max;
	};

	gObj<Buffer> Grid;
	gObj<Buffer> GridTmp;

	struct ScatteringParameters {
		float3 Sigma; float rm0;
		float3 G; float rm1;
		float3 Phi;
		float Pathtracer;
	};

	void SetScene(gObj<CA4G::Scene> scene) {
		IHasScene::SetScene(scene);
		if (sceneLoader != nullptr)
			sceneLoader->SetScene(scene);
	}

	void CreatingAssets(gObj<CopyingManager> manager) {

#pragma region DXR Pathtracing Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrPTPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPTPipeline->_Program->ProjToWorld = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->ParticipatingMedia = _ gCreate ConstantBuffer<ScatteringParameters>();

		dxrPTPipeline->_Program->DirectLighting = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		//dxrPTPipeline->_Program->Output = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		dxrPTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		dxrPTPipeline->_Program->Accum = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
#pragma endregion

		voxelizer->Vertices = sceneLoader->VertexBuffer;
		voxelizer->Transforms = sceneLoader->TransformBuffer;
		voxelizer->OB = sceneLoader->ObjectBuffer;

		Grid = _ gCreate RWStructuredBuffer<unsigned int>(gridSize * gridSize * gridSize / 8);
		GridTmp = _ gCreate RWStructuredBuffer<unsigned int>(gridSize * gridSize * gridSize / 8);

		voxelizer->GridInfo = _ gCreate ConstantBuffer<GridInfo>();
		dxrPTPipeline->_Program->GridInfo = voxelizer->GridInfo;
		spreading->GridInfo = voxelizer->GridInfo;
		gridToTexture->GridInfo = voxelizer->GridInfo;
		gridToTexture->DistanceField = _ gCreate DrawableTexture3D<float>(gridSize, gridSize, gridSize);

		float3 dim = sceneLoader->Scene->getMaximum() - sceneLoader->Scene->getMinimum();
		float maxDim = max(dim.x, max(dim.y, dim.z));

		manager gCopy ValueData(voxelizer->GridInfo, GridInfo{
			gridSize,
			sceneLoader->Scene->getMinimum() - float3(0.01, 0.01, 0.01),
			sceneLoader->Scene->getMinimum() + float3(maxDim, maxDim, maxDim) + float3(0.01, 0.01, 0.01)
			});
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

	void Frame() {

		static bool first = true;

		if (first) { // if dynamic scene this need to be done everyframe
			perform(BuildGrid);
			first = false;
		}

		perform(Pathtracing);
	}

	void BuildGrid(gObj<DXRManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

		voxelizer->Grid = Grid;

		compute gClear UAV(voxelizer->Grid, uint4(0));
		compute gSet Pipeline(voxelizer);
		compute gDispatch Threads((int)ceil(sceneLoader->VertexBuffer->ElementCount / 3.0 / CS_1D_GROUPSIZE));

		int radius = 1;
		//for (int level = 0; level < 2; level++)
		for (int level = 0; level < ceil(log(gridSize) / log(2)); level++)
		{
			compute gClear UAV(GridTmp, uint4(0));
			spreading->GridDst = GridTmp;
			spreading->GridSrc = Grid;
			compute gSet Pipeline(spreading);

			spreading->LevelInfo = int2(level, radius);
			compute gDispatch Threads(gridSize * gridSize * gridSize / CS_1D_GROUPSIZE);

			Grid = spreading->GridDst;
			GridTmp = spreading->GridSrc;
			if (level > 0)
				radius *= 2;
		}

		gridToTexture->GridSrc = Grid;
		compute gSet Pipeline(gridToTexture);
		compute gDispatch Threads(gridSize * gridSize * gridSize / CS_1D_GROUPSIZE);
		
		//dxrPTPipeline->_Program->Grid = Grid;
		dxrPTPipeline->_Program->Grid = gridToTexture->DistanceField;
	}

	float4x4 view, proj;

	void Pathtracing(gObj<DXRManager> manager) {

		static int FrameIndex = 0;

		auto rtProgram = dxrPTPipeline->_Program;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

			// Update Light intensity and position
			manager gCopy ValueData(rtProgram->LightingCB, Lighting{
					Light->Position, 0,
					Light->Intensity, 0,
					Light->Direction, 0
				});

			FrameIndex = 0;
			manager gClear UAV(rtProgram->Output, float4(0, 0, 0, 0));
			manager gClear UAV(rtProgram->Accum, float4(0, 0, 0, 0));

			manager gCopy ValueData(rtProgram->ProjToWorld, mul(view, proj).getInverse());

			manager gCopy ValueData(rtProgram->ParticipatingMedia, ScatteringParameters{
					this->extinction(), 0,
					this->gFactor, 0,
					this->phi(),
					this->pathtracing
				});
		}
		dxrPTPipeline->_Program->Debug = this->CountSteps ? 1 : 0;
		rtProgram->Frame = int2(FrameIndex, this->CountSteps ? 1 : 0);

		// Set DXR Pipeline
		manager gSet Pipeline(dxrPTPipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		int startTriangle;

#pragma region Single Pathtrace Stage

		static bool firstTime = true;

		if (firstTime) {
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

			firstTime = false;
		}

		// Setup a raygen shader
		manager gSet RayGeneration(dxrPTPipeline->PTMainRays);

		rtProgram->Frame = FrameIndex;

		if (FrameIndex < StopFrame || StopFrame == 0) {

			CurrentFrame = FrameIndex;

			// Dispatch primary rays
			manager gDispatch Rays(render_target->Width, render_target->Height);

			FrameIndex++;
		}

		manager gCopy All(render_target, rtProgram->Output);

		//auto compute = manager.Dynamic_Cast<ComputeManager>();
		//
		//filterPipeline->PassCount = FrameIndex;
		//compute gSet Pipeline(filterPipeline);
		//compute gDispatch Threads(
		//	(int)ceil(render_target->Width / CS_2D_GROUPSIZE),
		//	(int)ceil(render_target->Height / CS_2D_GROUPSIZE));

		////// Copy DXR output texture to the render target
		//manager gCopy All(render_target, filterPipeline->Final);
#pragma endregion
	}

};