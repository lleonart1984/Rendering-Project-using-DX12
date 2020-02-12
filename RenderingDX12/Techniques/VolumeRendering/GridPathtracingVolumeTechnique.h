#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class GridTrackingVolumeTechnique : public VolumeLoader, public IHasCamera, public IHasLight
{
	struct Accumulation : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\AccumulativeDensity_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Texture3D> Accumulation;

		gObj<Buffer> Transforms;

		void Globals() {
			UAV(0, Accumulation, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);

			CBV(0, Transforms, ShaderType_Any);

			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Any);
		}
	};

	struct MultiScatteringMarch : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\GridVolumePathtracing_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Texture3D> LightData;
		gObj<Texture3D> Majorants;

		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Output;

		gObj<Buffer> Camera;
		gObj<Buffer> VolumeInfo;
		gObj<Buffer> Transforms;
		gObj<Buffer> Lighting;
		int PassCount;

		void Globals() {
			UAV(0, Accumulation, ShaderType_Any);
			UAV(1, Output, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);
			SRV(1, LightData, ShaderType_Any);
			SRV(2, Majorants, ShaderType_Any);

			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Any);
			Static_SMP(1, Sampler::LinearWithoutMipMaps(), ShaderType_Any);

			CBV(0, Camera, ShaderType_Any);
			CBV(1, VolumeInfo, ShaderType_Any);
			CBV(2, Transforms, ShaderType_Any);
			CBV(3, Lighting, ShaderType_Any);
		}

		void Locals() {
			CBV(4, PassCount, ShaderType_Any);
		}
	};

	gObj<Accumulation> accumulation;
	gObj<MultiScatteringMarch> raymarch;

protected:

	int accSize = 512;

	float* majorants;

	void Startup() {
		VolumeLoader::Startup();

		int mWidth = (int)ceil(VolumeWidth / (float)MAJORANT_VOLUMEN_CELL_DIM);
		int mHeight = (int)ceil(VolumeHeight / (float)MAJORANT_VOLUMEN_CELL_DIM);
		int mSlices = (int)ceil(VolumeSlices / (float)MAJORANT_VOLUMEN_CELL_DIM);

		majorants = new float[mWidth * mHeight * mSlices];

		for (int z = 0; z < mSlices ; z++)
			for (int y =0; y < mHeight; y++)
				for (int x = 0; x < mWidth; x++)
				{
					float m = 0;

					for (int vz = z * MAJORANT_VOLUMEN_CELL_DIM; vz < min(z * MAJORANT_VOLUMEN_CELL_DIM + MAJORANT_VOLUMEN_CELL_DIM, VolumeSlices); vz++)
						for (int vy = y * MAJORANT_VOLUMEN_CELL_DIM; vy < min(y * MAJORANT_VOLUMEN_CELL_DIM + MAJORANT_VOLUMEN_CELL_DIM, VolumeHeight); vy++)
							for (int vx = x * MAJORANT_VOLUMEN_CELL_DIM; vx < min(x * MAJORANT_VOLUMEN_CELL_DIM + MAJORANT_VOLUMEN_CELL_DIM, VolumeWidth); vx++)
								m = max(m, Volume->data[vz * (VolumeWidth * VolumeHeight) + vy * VolumeWidth + vx]);

					majorants[z * (mWidth * mHeight) + y * mWidth + x] = m;
				}

		_ gLoad Pipeline(accumulation);
		_ gLoad Pipeline(raymarch);

		accumulation->Accumulation = _ gCreate DrawableTexture3D<float>(accSize, accSize, accSize);
		accumulation->VolumeData = VolumeData;
		accumulation->Transforms = _ gCreate ConstantBuffer<float4x4>();

		raymarch->Accumulation = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		raymarch->Camera = _ gCreate ConstantBuffer<float4x4>();
		raymarch->VolumeInfo = _ gCreate ConstantBuffer<VolumeInfo>();
		raymarch->VolumeData = VolumeData;
		raymarch->Majorants = _ gCreate ReadonlyTexture3D<float>(mWidth, mHeight, mSlices);
		raymarch->LightData = accumulation->Accumulation;
		raymarch->Transforms = _ gCreate ConstantBuffer<float4x4>();
		raymarch->Lighting = _ gCreate ConstantBuffer<Lighting>();

		perform(LoadMajorants);
	}

	void LoadMajorants(gObj<DXRManager> manager) {
		raymarch->Majorants->SetDebugName(L"Volume Data Buffer for sums");
		manager gCopy PtrData(raymarch->Majorants, majorants);
	}

	void Frame() {
		perform(DrawingVolume);
	}

	void DrawingVolume(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

#pragma region Density Accumulation from Light

		if (LightSourceIsDirty)
		{
			float3 lightDirection = -1 * normalize(Light->Direction);
			float3 corner = float3(VolumeWidth, VolumeHeight, VolumeSlices) / max(VolumeWidth, max(VolumeHeight, VolumeSlices));
			float size = length(corner);
			float3 accY = normalize(cross(
				dot(corner, lightDirection) < size ? corner : float3(corner.y, corner.z, corner.x),
				lightDirection)) * size;
			float3 accZ = lightDirection * size;
			float3 accX = normalize(cross(accZ, accY)) * size;

			float4x4 centering = float4x4(
				2, 0, 0, 0,
				0, 2, 0, 0,
				0, 0, 2, 0,
				-1, -1, -1, 1
			);
			float4x4 volumeScaling = float4x4(
				corner.x, 0, 0, 0,
				0, corner.y, 0, 0,
				0, 0, corner.z, 0,
				0, 0, 0, 1
			);

			// from accumulation to volume
			float4x4 lightRotation = float4x4(
				accX.x, accX.y, accX.z, 0,
				accY.x, accY.y, accY.z, 0,
				accZ.x, accZ.y, accZ.z, 0,
				0, 0, 0, 1
			);
			float4x4 fromVolToLight = mul(centering, mul(volumeScaling, mul(lightRotation.getInverse(), centering.getInverse())));

			compute gCopy ValueData(accumulation->Transforms, fromVolToLight.getInverse());
			compute gSet Pipeline(accumulation);
			compute gDispatch Threads(accSize / CS_2D_GROUPSIZE, accSize / CS_2D_GROUPSIZE);
			compute gCopy ValueData(raymarch->Transforms, fromVolToLight);
		}
#pragma endregion

		static int FrameIndex = 0;

		raymarch->PassCount = FrameIndex++;

		if (CameraIsDirty || LightSourceIsDirty)
		{
			FrameIndex = 0;

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
		}

		compute gSet Pipeline(raymarch);

		compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));

		compute gCopy All(render_target, raymarch->Output);
	}
};