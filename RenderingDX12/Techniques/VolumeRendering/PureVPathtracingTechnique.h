#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class PureVPathtracingTechnique : public VolumeLoader, public IHasCamera, public IHasLight
{

	struct MultiScatteringMarch : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\PurePathtracing_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Output;

		gObj<Buffer> Camera;
		gObj<Buffer> VolumeInfo;
		gObj<Buffer> Lighting;
		int PassCount;

		void Globals() {
			UAV(0, Accumulation, ShaderType_Any);
			UAV(1, Output, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);

			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Any);

			CBV(0, Camera, ShaderType_Any);
			CBV(1, VolumeInfo, ShaderType_Any);
			CBV(2, Lighting, ShaderType_Any);
		}

		void Locals() {
			CBV(3, PassCount, ShaderType_Any);
		}
	};

	gObj<MultiScatteringMarch> raymarch;

protected:

	void Startup() {
		VolumeLoader::Startup();

		_ gLoad Pipeline(raymarch);

		raymarch->Accumulation = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		raymarch->Camera = _ gCreate ConstantBuffer<float4x4>();
		raymarch->VolumeInfo = _ gCreate ConstantBuffer<VolumeInfo>();
		raymarch->VolumeData = VolumeData;
		raymarch->Lighting = _ gCreate ConstantBuffer<Lighting>();
	}

	void Frame() {
		perform(DrawingVolume);
	}

	void DrawingVolume(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

		static int FrameIndex = 0;

		if (CameraIsDirty || LightSourceIsDirty)
			FrameIndex = 0;

		raymarch->PassCount = FrameIndex++;

		float4x4 proj, view;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
		compute gCopy ValueData(raymarch->Camera, mul(proj.getInverse(), view.getInverse()));
		compute gCopy ValueData(raymarch->VolumeInfo, VolumeInfo{
			VolumeWidth,
			VolumeHeight,
			VolumeSlices,
			densityScale,
			globalAbsortion
			});
		compute gCopy ValueData(raymarch->Lighting,
			Lighting{
				Light->Position, 0,
				Light->Intensity, 0,
				Light->Direction, 0
			});

		compute gSet Pipeline(raymarch);

		compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));

		compute gCopy All(render_target, raymarch->Output);
	}
};