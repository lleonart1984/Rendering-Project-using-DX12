#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"
#include "../VolumePathtracing/VolDirectLightingTechnique.h"

struct VolSTPathtracerWithDFTechnique : public DirectLightingTechnique, public IHasScatteringEvents, public IHasAccumulative {
public:

	~VolSTPathtracerWithDFTechnique() {
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

	// DXR pipeline for pathtracing stage
	struct DXR_PT_Pipeline : public RTPipelineManager {
		gObj<RayGenerationHandle> PTMainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> PTScattering;
		gObj<HitGroupHandle> PTMaterial;

		class DXR_RT_IL : public DXIL_Library<DXR_PT_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\VolSTPathtracerWithDF_RT.cso"));

				_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->PTScattering, L"PTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_PT_Pipeline> {
			void Setup() {
				_ gSet Payload(4 * 3 * 4 + 4 + 4 + 8); // 4 float3 + int + 2 uint
				_ gSet StackSize(1); // No recursion needed!
				_ gLoad Shader(Context()->PTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->PTMaterial, Context()->PTScattering, nullptr, nullptr);
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

			gObj<Buffer> Grid;

			gObj<Texture2D>* Textures;
			int TextureCount;

			gObj<Buffer> CameraCB;
			gObj<Buffer> LightingCB;
			gObj<Buffer> LightTransforms;
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

				SRV(3, Positions);
				SRV(4, Normals);
				SRV(5, Coordinates);
				SRV(6, MaterialIndices);

				SRV(7, LightPositions);

				SRV(8, DirectLighting);

				SRV(9, Grid);

				SRV_Array(10, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());
				Static_SMP(1, Sampler::LinearWithoutMipMaps());

				CBV(0, CameraCB);
				CBV(1, LightingCB);
				CBV(2, LightTransforms);
				CBV(3, Frame);
				CBV(4, ParticipatingMedia);
				CBV(5, ProjToWorld);
				CBV(6, GridInfo);
				CBV(7, Debug);
			}

			void HitGroup_Locals() {
				CBV(8, CurrentObjectInfo);
			}
		};
		gObj<DXR_RT_Program> _Program;

		void Setup() override
		{
			_ gLoad Library(_Library);
			_ gLoad Program(_Program);
		}
	};


	struct Filter : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\Pathtracing\\Filter_CS.cso"));
		}

		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Background;
		gObj<Texture2D> Positions;
		gObj<Texture2D> Normals;
		gObj<Texture2D> Coordinates;
		gObj<Texture2D> MaterialIndices;
		gObj<Buffer> Materials;
		gObj<Texture2D>* Textures;
		int TextureCount;

		gObj<Texture2D> Final;

		int PassCount;

		void Globals() {
			SRV(0, Accumulation, ShaderType_Any);
			SRV(1, Background, ShaderType_Any);
			SRV(2, Positions, ShaderType_Any);
			SRV(3, Normals, ShaderType_Any);
			SRV(4, Coordinates, ShaderType_Any);
			SRV(5, MaterialIndices, ShaderType_Any);
			SRV(6, Materials, ShaderType_Any);
			SRV_Array(7, Textures, TextureCount, ShaderType_Any);

			Static_SMP(0, Sampler::Linear(), ShaderType_Any);

			UAV(0, Final, ShaderType_Any);

			CBV(0, PassCount, ShaderType_Any);
		}
	};

	gObj<Voxelizer> voxelizer;
	gObj<Spreading> spreading;
	gObj<DXR_PT_Pipeline> dxrPTPipeline;
	gObj<Filter> filterPipeline;

	void Startup() {

		DirectLightingTechnique::Startup();

		wait_for(signal(flush_all_to_gpu));

		_ gLoad Pipeline(voxelizer);
		_ gLoad Pipeline(spreading);
		_ gLoad Pipeline(dxrPTPipeline);
		_ gLoad Pipeline(filterPipeline);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		perform(CreateSceneOnGPU);
	}

	int gridSize = 256;
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
	
	void CreatingAssets(gObj<CopyingManager> manager) {

#pragma region DXR Pathtracing Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrPTPipeline->_Program->CameraCB = computeDirectLighting->ViewTransform;
		dxrPTPipeline->_Program->LightingCB = computeDirectLighting->Lighting;
		dxrPTPipeline->_Program->LightTransforms = computeDirectLighting->LightTransforms;
		dxrPTPipeline->_Program->ProjToWorld = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->ParticipatingMedia = _ gCreate ConstantBuffer<ScatteringParameters>();

		dxrPTPipeline->_Program->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		dxrPTPipeline->_Program->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		dxrPTPipeline->_Program->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		dxrPTPipeline->_Program->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;
		dxrPTPipeline->_Program->LightPositions = gBufferFromLight->pipeline->GBuffer_P;

		dxrPTPipeline->_Program->DirectLighting = DirectLighting;
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

		float3 dim = sceneLoader->Scene->getMaximum() - sceneLoader->Scene->getMinimum();
		float maxDim = max(dim.x, max(dim.y, dim.z));

		manager gCopy ValueData(voxelizer->GridInfo, GridInfo{
			gridSize,
			sceneLoader->Scene->getMinimum(),
			sceneLoader->Scene->getMinimum() + float3(maxDim, maxDim, maxDim)
			});

		filterPipeline->Accumulation = dxrPTPipeline->_Program->Accum;
		filterPipeline->Background = DirectLighting;
		filterPipeline->Positions = computeDirectLighting->Positions;
		filterPipeline->Normals = computeDirectLighting->Normals;
		filterPipeline->Coordinates = computeDirectLighting->Coordinates;
		filterPipeline->MaterialIndices = computeDirectLighting->MaterialIndices;
		filterPipeline->Materials = computeDirectLighting->Materials;
		filterPipeline->TextureCount = computeDirectLighting->TextureCount;
		filterPipeline->Textures = computeDirectLighting->Textures;
		filterPipeline->Final = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
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

		DirectLightingTechnique::Frame();

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
		for (int level = 0; level < ceil(log(gridSize)/log(2)); level++)
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

		dxrPTPipeline->_Program->Grid = Grid;
	}

	void Pathtracing(gObj<DXRManager> manager) {

		static int FrameIndex = 0;

		auto rtProgram = dxrPTPipeline->_Program;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			FrameIndex = 0;
			manager gClear UAV(rtProgram->Output, float4(0, 0, 0, 0));
			manager gClear UAV(rtProgram->Accum, float4(0, 0, 0, 0));

			manager gCopy ValueData(rtProgram->ProjToWorld, mul(view, proj).getInverse());
		
			manager gCopy ValueData(rtProgram->ParticipatingMedia, ScatteringParameters{
					this->density * this->scatteringAlbedo, 0,
					this->gFactor, 0,
					this->phi,
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