#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class BasicVolumeTechnique : public VolumeLoader, public IHasCamera
{

	struct BasicRayMarch : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\BasicRayMarch_CS.cso"));
		}

		gObj<Buffer> VolumeData;
		gObj<Texture2D> Output;
		gObj<Buffer> Camera;
		gObj<Buffer> VolumeInfo;

		void Globals() {
			UAV(0, Output, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);

			CBV(0, Camera, ShaderType_Any);
			CBV(1, VolumeInfo, ShaderType_Any);
		}
	};

	gObj<BasicRayMarch> basicViewing;

protected:
	void Startup() {
		VolumeLoader::Startup();

		_ gLoad Pipeline(basicViewing);

		basicViewing->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		basicViewing->Camera = _ gCreate ConstantBuffer<float4x4>();
		basicViewing->VolumeInfo = _ gCreate ConstantBuffer<VolumeInfo>();
		basicViewing->VolumeData = VolumeData;
	}

	void Frame() {
		perform(DrawingVolume);
	}

	void DrawingVolume(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

		float4x4 proj, view;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		compute gCopy ValueData(basicViewing->Camera, mul(proj.getInverse(), view.getInverse()));
		compute gCopy ValueData(basicViewing->VolumeInfo, VolumeInfo{
			VolumeWidth,
			VolumeHeight,
			VolumeSlices,
			globalAbsortion 
			});

		compute gSet Pipeline(basicViewing);

		compute gDispatch Threads((int)ceil(render_target->Width / CS_2D_GROUPSIZE), (int)ceil(render_target->Height / CS_2D_GROUPSIZE));

		compute gCopy All(render_target, basicViewing->Output);
	}
};