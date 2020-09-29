#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class SphereScatteringTestTechnique : public Technique, public IHasCamera, public IHasLight, public IHasScatteringEvents
{

	struct MultiScatteringMarch : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\SphereSubsurfaceScattering_CS.cso"));
			//_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\SphereVolPathtracing_CS.cso"));
		}

		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Output;

		gObj<Buffer> Camera;
		gObj<Buffer> ScatteringInfo;
		int CountSteps;
		gObj<Buffer> Lighting;
		int PassCount;

		void Globals() {
			UAV(0, Accumulation, ShaderType_Any);
			UAV(1, Output, ShaderType_Any);

			CBV(0, Camera, ShaderType_Any);
			CBV(1, ScatteringInfo, ShaderType_Any);
			CBV(2, CountSteps, ShaderType_Any);
			CBV(3, Lighting, ShaderType_Any);
		}

		void Locals() {
			CBV(4, PassCount, ShaderType_Any);
		}
	};

	gObj<MultiScatteringMarch> raymarch;

protected:

	int accSize = 512;

	struct ScatteringParameters {
		float3 Sigma; float rm0;
		float3 G; float rm1;
		float3 Phi; 
		float Pathtracer;
	};


	void Startup() {
		_ gLoad Pipeline(raymarch);

		raymarch->Accumulation = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		raymarch->Camera = _ gCreate ConstantBuffer<float4x4>();
		raymarch->ScatteringInfo = _ gCreate ConstantBuffer<ScatteringParameters>();
		raymarch->Lighting = _ gCreate ConstantBuffer<Lighting>();
	}

	void Frame() {
		perform(DrawingVolume);
	}

	
	void DrawingVolume(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

		static int FrameIndex = 0;

		raymarch->PassCount = FrameIndex++;
		raymarch->CountSteps = CountSteps ? 1 : 0;
		if (CameraIsDirty || LightSourceIsDirty)
		{
			FrameIndex = 0;

			compute gClear UAV(raymarch->Accumulation, uint4(0));

			float4x4 proj, view;
			Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
			compute gCopy ValueData(raymarch->Camera, mul(proj.getInverse(), view.getInverse()));
			compute gCopy ValueData(raymarch->ScatteringInfo, ScatteringParameters{
				this->extinction(), 0,
				this->gFactor, 0,
				this->phi(),
				this->pathtracing });

			compute gCopy ValueData(raymarch->Lighting,
				Lighting{
					Light->Position, 0,
					Light->Intensity, 0,
					Light->Direction, 0
				});
		}

		compute gSet Pipeline(raymarch);

		compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));

		compute gCopy All(render_target, raymarch->Output);
	}
};