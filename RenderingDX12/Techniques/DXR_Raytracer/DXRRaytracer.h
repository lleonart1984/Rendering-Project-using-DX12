#pragma once

class DXRBasicSceneRaytracer : public Technique, public IHasScene, public IHasCamera, public IHasLight {
public:

	struct DXR_Pipeline : public RTPipelineManager {
		// Inherited via RTPipelineManager

		gObj<RayGenerationHandle> MainRays;
		gObj<MissHandle> EnvironmentMap;
		gObj<ClosestHitHandle> FresnelScattering;
		gObj<ClosestHitHandle> LambertScattering;
		gObj<HitGroupHandle> FresnelMaterial;
		gObj<HitGroupHandle> LambertMaterial;

		class DXR_Basic_IL : public DXIL_Library<DXR_Pipeline> {
			void Setup() {
				_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\DXR_Raytracer\\DXR_RTX_Basic_RT.cso"));

				_ gLoad Shader(Context()->MainRays, L"MainRays");
				_ gLoad Shader(Context()->EnvironmentMap, L"EnvironmentMap");
				_ gLoad Shader(Context()->FresnelScattering, L"FresnelScattering");
				_ gLoad Shader(Context()->LambertScattering, L"LambertScattering");
			}
		};
		gObj<DXR_Basic_IL> _Library;

		struct DXR_RT_Program : public RTProgram<DXR_Pipeline> {
			void Setup() {
				_ gSet Payload(16);
				_ gSet StackSize(5);
				_ gLoad Shader(Context()->MainRays);
				_ gLoad Shader(Context()->EnvironmentMap);
				_ gCreate HitGroup(Context()->FresnelMaterial, Context()->FresnelScattering, nullptr, nullptr);
				_ gCreate HitGroup(Context()->LambertMaterial, Context()->LambertScattering, nullptr, nullptr);
			}

			gObj<SceneOnGPU> Scene;
			gObj<Buffer> Vertices;
			gObj<Buffer> Materials;
			gObj<Texture2D> *Textures;
			int TextureCount;
			gObj<Buffer> CameraCB;
			
			gObj<Texture2D> ScreenOutput;

			struct ObjInfo {
				int TriangleOffset;
				int MaterialIndex;
			} CurrentObjectInfo;

			void Globals (){
				UAV(0, ScreenOutput);
				ADS(0, Scene);
				SRV(1, Vertices);
				SRV(2, Materials);
				SRV_Array(3, Textures, TextureCount);

				Static_SMP(0, Sampler::Linear());

				CBV(0, CameraCB);
			}

			void HitGroup_Locals() {
				CBV(1, CurrentObjectInfo);
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
	gObj<Texture2D>* Textures;
	int TextureCount;
	gObj<SceneOnGPU> SceneOnGPU;
	gObj<Buffer> VB;
	gObj<Buffer> Vertices;
	gObj<Buffer> MaterialsB;
	gObj<Buffer> CameraCB;

	gObj<DXR_Pipeline> Pipeline;
	gObj<Texture2D> rtRenderTarget;

	void Startup() {
		perform(LoadingSceneAssets);
		perform(CreateSceneOnGPU);
		_ gLoad Pipeline(Pipeline);

		// Bind some global fields from here...
		Pipeline->_Program->Scene = SceneOnGPU;
		Pipeline->_Program->Materials = MaterialsB;
		Pipeline->_Program->Vertices = Vertices;
		Pipeline->_Program->ScreenOutput = rtRenderTarget;
		Pipeline->_Program->Textures = Textures;
		Pipeline->_Program->TextureCount = TextureCount;
		Pipeline->_Program->CameraCB = CameraCB;
	}

	void LoadingSceneAssets(gObj<CopyingManager> manager) {
		// loading scene textures
		TextureCount = Scene->Textures().size();
		Textures = new gObj<Texture2D>[TextureCount];
		for (int i = 0; i < Scene->Textures().size(); i++)
			manager gLoad FromData(Textures[i], Scene->Textures()[i]);

		// load full vertex buffer of all scene geometries
		VB = _ gCreate GenericBuffer<SCENE_VERTEX>(D3D12_RESOURCE_STATE_GENERIC_READ, Scene->VerticesCount(), CPU_WRITE_GPU_READ);
		manager gCopy PtrData(VB, Scene->Vertices());
		
		// SRV to bind to the GPU for vertices accessing
		// Must be used VB as well, but I got an error,
		// This is a work around
		Vertices = _ gCreate StructuredBuffer<SCENE_VERTEX>(Scene->VerticesCount());
		manager gCopy PtrData(Vertices, Scene->Vertices());

		MaterialsB = _ gCreate StructuredBuffer<SCENE_MATERIAL>(Scene->MaterialsCount());
		manager gCopy PtrData(MaterialsB, &Scene->Materials().first());

		CameraCB = _ gCreate ConstantBuffer<float4x4>();
		// CameraCB will be updated every frame

		rtRenderTarget = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
	}

	void CreateSceneOnGPU(gObj<DXRManager> manager) {
		/// Loads a static scene for further ray-tracing
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
		this->SceneOnGPU = instances gCreate BakedScene();
	}

	void Frame() {
		perform(Raytracing);
	}

	void Raytracing(gObj<DXRManager> manager) {

		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
		float4x4 projToWorld = mul(view, proj).getInverse();
		
		auto rtProgram = Pipeline->_Program;

		// Update camera
		manager gCopy ValueData(rtProgram->CameraCB, projToWorld);
		
		// Set DXR Pipeline
		manager gSet Pipeline(Pipeline);
		// Activate program with main shaders
		manager gSet Program(rtProgram);

		// Set Miss in slot 0
		manager gSet Miss(Pipeline->EnvironmentMap, 0);

		// Setup a simple hitgroup per object
		// each object knows the offset in triangle buffer
		// and the material index for further light scattering
		int startTriangle = 0;
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto sceneObject = Scene->Objects()[i];

			rtProgram->CurrentObjectInfo.TriangleOffset = startTriangle;
			rtProgram->CurrentObjectInfo.MaterialIndex = Scene->MaterialIndices()[i];

			if (sceneObject.Material->Roulette.z > 0) // Fresnel scattering
				manager gSet HitGroup(Pipeline->FresnelMaterial, i);
			else
				manager gSet HitGroup(Pipeline->LambertMaterial, i);

			startTriangle += sceneObject.vertexesCount / 3;
		}
		
		// Setup a raygen shader
		manager gSet RayGeneration(Pipeline->MainRays);
		
		// Dispatch primary rays
		manager gDispatch Rays(render_target->Width, render_target->Height);
		
		// Copy DXR output texture to the render target
		manager gCopy All(render_target, rtRenderTarget);
	}
};