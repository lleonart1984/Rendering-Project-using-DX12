#pragma once

#include "../../Techniques/GUI_Traits.h"
#include "../DeferredShading/GBufferConstruction.h"
#include "../CommonGI/Parameters.h"

class VolDirectLightingTechnique : public Technique, public IHasScene, public IHasLight, public IHasCamera, public IHasScatteringEvents {
public:
	// Scene loading process to retain scene on the GPU
	gObj<RetainedSceneLoader> sceneLoader;
	// GBuffer process used to build GBuffer data from light
	gObj<GBufferConstruction> gBufferFromLight;
	// GBuffer process used to build GBuffer data from viewer
	gObj<GBufferConstruction> gBufferFromViewer;
	// Output texture with direct lighting compute
	gObj<Texture2D> DirectLighting;

	struct DeferredDirectLightingCS : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumePathtracing\\VolDeferredLight_CS.cso"));
		}

		gObj<Texture2D> Positions;
		gObj<Texture2D> Normals;
		gObj<Texture2D> Coordinates;
		gObj<Texture2D> MaterialIndices;
		gObj<Texture2D> ShadowMap;

		gObj<Buffer> Materials;
		gObj<Texture2D>* Textures;
		int TextureCount;

		gObj<Buffer> Lighting;
		gObj<Buffer> ViewTransform;
		gObj<Buffer> LightTransforms;
		gObj<Buffer> ParticipatingMedia;


		gObj<Texture2D> DirectLighting;

		void Globals() {
			UAV(0, DirectLighting, ShaderType_Any);

			SRV(0, Positions, ShaderType_Any);
			SRV(1, Normals, ShaderType_Any);
			SRV(2, Coordinates, ShaderType_Any);
			SRV(3, MaterialIndices, ShaderType_Any);
			SRV(4, ShadowMap, ShaderType_Any);
			SRV(5, Materials, ShaderType_Any);
			SRV_Array(6, Textures, TextureCount, ShaderType_Any);

			Static_SMP(0, Sampler::Linear(), ShaderType_Any);
			Static_SMP(1, Sampler::LinearWithoutMipMaps(), ShaderType_Any);

			CBV(0, Lighting, ShaderType_Any);
			CBV(1, ViewTransform, ShaderType_Any);
			CBV(2, LightTransforms, ShaderType_Any);
			CBV(3, ParticipatingMedia, ShaderType_Any);
		}
	};

	gObj<DeferredDirectLightingCS> computeDirectLighting;

	struct ScatteringParameters {
		float3 Sigma; float rm0;
		float3 G; float rm1;
		float3 Phi;
		float Pathtracer;
	};

	void Startup() {
		// Load and setup scene loading process
		sceneLoader = new RetainedSceneLoader();
		sceneLoader->SetScene(this->Scene);
		_ gLoad Subprocess(sceneLoader);

		// Load and setup gbuffer construction process from light
		gBufferFromLight = new GBufferConstruction(SHADOWMAP_DIMENSION, SHADOWMAP_DIMENSION);
		gBufferFromLight->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromLight);

		// Load and setup gbuffer construction process from viewer
		gBufferFromViewer = new GBufferConstruction(render_target->Width, render_target->Height);
		gBufferFromViewer->sceneLoader = this->sceneLoader;
		_ gLoad Subprocess(gBufferFromViewer);

		_ gLoad Pipeline(computeDirectLighting);

		DirectLighting = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);

		computeDirectLighting->Positions = gBufferFromViewer->pipeline->GBuffer_P;
		computeDirectLighting->Normals = gBufferFromViewer->pipeline->GBuffer_N;
		computeDirectLighting->Coordinates = gBufferFromViewer->pipeline->GBuffer_C;
		computeDirectLighting->MaterialIndices = gBufferFromViewer->pipeline->GBuffer_M;

		computeDirectLighting->ShadowMap = gBufferFromLight->pipeline->GBuffer_P;

		computeDirectLighting->Textures = sceneLoader->Textures;
		computeDirectLighting->TextureCount = sceneLoader->TextureCount;
		computeDirectLighting->Materials = sceneLoader->MaterialBuffer;

		computeDirectLighting->ViewTransform = _ gCreate ConstantBuffer<float4x4>();
		computeDirectLighting->LightTransforms = gBufferFromLight->getGlobalTransforms();
		computeDirectLighting->Lighting = _ gCreate ConstantBuffer<Lighting>();
		computeDirectLighting->ParticipatingMedia = _ gCreate ConstantBuffer<ScatteringParameters>();

		computeDirectLighting->DirectLighting = DirectLighting;
	}

	void SetScene(gObj<CA4G::Scene> scene) {
		IHasScene::SetScene(scene);
		if (sceneLoader != nullptr)
			sceneLoader->SetScene(scene);
	}

	float4x4 view, proj;
	float4x4 lightView, lightProj;

	void Frame() {

		if (CameraIsDirty) {
#pragma region Construct GBuffer from viewer
			Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

			gBufferFromViewer->ViewMatrix = view;
			gBufferFromViewer->ProjectionMatrix = proj;
			ExecuteFrame(gBufferFromViewer);
#pragma endregion
		}

		if (LightSourceIsDirty)
		{
#pragma region Construct GBuffer from light
			lightView = LookAtLH(this->Light->Position, this->Light->Position + float3(0, -1, 0), float3(0, 0, 1));
			lightProj = PerspectiveFovLH(PI / 2, 1, 0.001f, 10);
			gBufferFromLight->ViewMatrix = lightView;
			gBufferFromLight->ProjectionMatrix = lightProj;
			ExecuteFrame(gBufferFromLight);
#pragma endregion
		}

		perform(DrawDeferredShading);
	}

	void DrawDeferredShading(gObj<DXRManager> dxrmanager) {

		auto manager = dxrmanager.Dynamic_Cast<ComputeManager>();

		if (CameraIsDirty) {
			// Update camera
			// Required for shadow mapping technique to get everything in world space
			manager gCopy ValueData(computeDirectLighting->ViewTransform, view.getInverse());
		}

		if (LightSourceIsDirty) {
			// Update Light intensity and position
			manager gCopy ValueData(computeDirectLighting->Lighting, Lighting{
					Light->Position, 0,
					Light->Intensity, 0,
					Light->Direction, 0
				});

			manager gCopy ValueData(computeDirectLighting->ParticipatingMedia, ScatteringParameters{
				this->extinction(), 0,
				this->gFactor, 0,
				this->phi(),
				this->pathtracing
				});

		}

		manager gClear UAV(DirectLighting, float4(0, 0, 0, 1));

		manager gSet Pipeline(computeDirectLighting);
		manager gDispatch Threads(ceil(render_target->Width / (double)CS_2D_GROUPSIZE), ceil(render_target->Height / (double)CS_2D_GROUPSIZE));
	}
};