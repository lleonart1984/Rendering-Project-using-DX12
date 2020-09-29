#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"
#include "../VolumePathtracing/VolDirectLightingTechnique.h"

struct VolPurePTTechnique : public Technique, public IHasScene, public IHasLight, public IHasCamera, public IHasScatteringEvents, public IHasAccumulative {
public:
	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;

	~VolPurePTTechnique() {
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
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\VolPurePathtracing_RT.cso"));

				_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->PTScattering, L"PTScattering");
			}
		};
		gObj<DXR_RT_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_PT_Pipeline> {
			void Setup() {
				_ gSet Payload(4 * 15); 
				_ gSet StackSize(1); // No recursion needed!
				_ gLoad Shader(Context()->PTMainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->PTMaterial, Context()->PTScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;

			gObj<Buffer> Grid;

			gObj<Texture2D>* Textures;
			int TextureCount;

			gObj<Buffer> LightingCB;
			int2 Frame;
			gObj<Buffer> ParticipatingMedia;
			gObj<Buffer> ProjToWorld;

			gObj<Buffer> GridInfo;
			int Debug;

			gObj<Buffer> ModelSlices;

			gObj<Texture2D> Background;
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
				SRV(3, Background);

				SRV(4, Grid);

				SRV_Array(5, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());

				CBV(0, ProjToWorld);
				CBV(1, LightingCB);
				CBV(2, Frame);
				CBV(4, ParticipatingMedia);
				CBV(5, GridInfo);
				CBV(6, Debug);

				CBV(7, ModelSlices);
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
		// Load and setup scene loading process
		sceneLoader = new RetainedSceneLoader();
		sceneLoader->SetScene(this->Scene);
		_ gLoad Subprocess(sceneLoader);

		wait_for(signal(flush_all_to_gpu));

		_ gLoad Pipeline(voxelizer);
		_ gLoad Pipeline(spreading);
		_ gLoad Pipeline(dxrPTPipeline);
		_ gLoad Pipeline(filterPipeline);

		// Load assets to render the deferred lighting image
		perform(CreatingAssets);

		perform(CreateSceneOnGPU);
	}

	int gridSize = 1024;
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

	struct ModelSlicesCB {
		float Slices[1532];
	};

	ModelSlicesCB slices = {
-1.4647857,-0.42826492,0.0013390315,-0.027267234,1.9631772,0.23757873,-0.0020370884,0.3737895,-1.8965818,-0.26607674,0.0007524612,-0.34710613,-1.5985893,-0.21281911,-0.00064108754,-0.2896381,0.7190652,-1.7997388,1.5423869,1.3695614,-0.5154928,-8.022841,0.2165962,1.2858816,0.27903977,-7.2458677,0.8634012,-0.18221265,0.42385444,-10.611959,1.2483487,0.99954826,0.18819262,-6.2749906,0.3309136,-7.3158817,0.38619936,0.3705537,0.63924336,0.13597777,0.28611255,0.8962767,0.9290795,-2.5632136,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-8.774182e-05,0.0,0.0,0.0,0.9255522,0.41462663,0.2256235,0.00414795,-0.7786284,-1.1499567,-0.30250007,0.0020423084,1.0476426,0.3274382,0.13693495,-0.004216575,0.19701533,-0.7395321,0.4402949,-0.00011078982,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,1.217227,-0.7776381,0.9538226,0.20390716,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.1635268,0.47841036,0.3801087,-0.28394175,-0.18315592,-0.6865864,-0.27191117,-0.20040253,-0.67976075,-0.48755664,-0.54452306,-0.63608444,0.13916163,0.06282353,-0.8440717,-0.2327592,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.3527436,-0.15526,-0.023627017,0.83549196,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.09213692,0.058302205,-0.0231775,0.42448258,-0.31112236,-0.3370085,0.27191323,-0.723563,0.14158297,0.20348573,0.09355828,0.39322242,-0.70731956,-0.46822667,-0.2398889,-0.2416297,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.22911927,-0.120498806,-0.026487859,-0.21361564,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.46676952,0.23806046,-0.16086742,0.08300621,0.21523386,-0.12176295,-0.30000535,0.9243173,0.08341274,0.58993834,-0.029834576,0.13168065,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.17423737,-0.1276763,-0.117987655,0.0,-0.22125627,0.03388198,-0.1346194,0.04082939,-0.8635041,-0.4739341,-0.1708617,-0.4937302,-0.314263,-0.61796826,-0.080566004,-0.08501824,0.18831664,0.33234015,-0.05323136,0.08699551,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.17818955,-0.32257217,0.65886194,0.6248311,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.021694936,-0.0073400754,0.003928341,0.02739275,0.19251576,0.051194604,-0.053545944,-0.09787993,0.03763834,0.5699301,0.008806155,-1.2103598,0.77712303,0.32723936,0.06403976,-0.02887953,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,1.4667647,-0.21735749,0.4467751,-1.4974895,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.47017896,-0.020598216,-0.022962948,0.31709784,0.338083,-0.44989046,-0.074715964,0.66618264,-0.4524785,-0.23476051,-0.044662375,-0.18959358,0.16153713,0.32541636,0.16546361,1.2241935,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.5745212,0.0048284153,-0.5753595,0.23922971,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.17851326,0.13587114,-0.03003536,0.24555103,-0.021877235,-0.26714474,0.02069091,0.14613815,0.011808174,0.49991292,0.0012738878,-0.22097436,0.19420603,-0.055961106,-0.23957144,0.0996349,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,-0.14149353,-0.10642349,-0.4737218,0.18177813,0.0,0.0,0.0,0.0
	};

	void CreatingAssets(gObj<CopyingManager> manager) {

#pragma region DXR Pathtracing Pipeline Objects
		dxrPTPipeline->_Program->TextureCount = sceneLoader->TextureCount;
		dxrPTPipeline->_Program->Textures = sceneLoader->Textures;
		dxrPTPipeline->_Program->Materials = sceneLoader->MaterialBuffer;
		dxrPTPipeline->_Program->Vertices = sceneLoader->VertexBuffer;
		dxrPTPipeline->_Program->LightingCB = _ gCreate ConstantBuffer<Lighting>();
		dxrPTPipeline->_Program->ProjToWorld = _ gCreate ConstantBuffer<float4x4>();
		dxrPTPipeline->_Program->ParticipatingMedia = _ gCreate ConstantBuffer<ScatteringParameters>();

		//dxrPTPipeline->_Program->Output = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		dxrPTPipeline->_Program->Background = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		dxrPTPipeline->_Program->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		dxrPTPipeline->_Program->Accum = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);

		dxrPTPipeline->_Program->ModelSlices = _ gCreate ConstantBuffer<ModelSlicesCB>();
		manager gCopy ValueData(dxrPTPipeline->_Program->ModelSlices, slices);
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

		dxrPTPipeline->_Program->Grid = Grid;
	}

	void Pathtracing(gObj<DXRManager> manager) {
		static int FrameIndex = 0;

		auto rtProgram = dxrPTPipeline->_Program;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			float4x4 view, proj;
			Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

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

			// Update Light intensity and position
			manager gCopy ValueData(rtProgram->LightingCB, Lighting{
					Light->Position, 0,
					Light->Intensity, 0,
					Light->Direction, 0
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