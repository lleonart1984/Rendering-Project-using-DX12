#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class FilteredVolumePathtracingTechnique : public VolumeLoader, public IHasCamera, public IHasLight
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
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\VolumePathtraceWithGBuffer_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Texture3D> LightData;
		
		gObj<Texture2D> Radiance;
		gObj<Texture2D> Positions;
		gObj<Texture2D> Directions;
		gObj<Texture2D> Gradient;

		gObj<Buffer> Camera;
		gObj<Buffer> VolumeInfo;
		gObj<Buffer> Transforms;
		gObj<Buffer> Lighting;
		int PassCount;

		void Globals() {
			UAV(0, Radiance, ShaderType_Any);
			UAV(1, Positions, ShaderType_Any);
			UAV(2, Directions, ShaderType_Any);
			UAV(3, Gradient, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);
			SRV(1, LightData, ShaderType_Any);

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

	struct Wavelets : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\Wavelet_CS.cso"));
		}

		gObj<Texture2D> Radiance;
		gObj<Texture2D> Positions;
		gObj<Texture2D> Directions;
		gObj<Texture2D> Gradient;

		gObj<Texture2D> Input;
		gObj<Texture2D> Output;

		int2 Info;

		void Globals() {
			SRV(0, Input, ShaderType_Any);
			
			SRV(1, Radiance, ShaderType_Any);
			SRV(2, Positions, ShaderType_Any);
			SRV(3, Directions, ShaderType_Any);
			SRV(4, Gradient, ShaderType_Any);
			
			UAV(0, Output, ShaderType_Any);
			
			CBV(0, Info, ShaderType_Any);
		}
	};

	struct Filtering : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\Filter_CS.cso"));
		}

		gObj<Texture2D> Radiance;
		gObj<Texture2D> Positions;

		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Output;

		int PassCount;

		void Globals() {
			SRV(0, Radiance, ShaderType_Any);
			SRV(1, Positions, ShaderType_Any);

			UAV(0, Accumulation, ShaderType_Any);
			UAV(1, Output, ShaderType_Any);
		}

		void Locals() {
			CBV(0, PassCount, ShaderType_Any);
		}
	};

	gObj<Accumulation> accumulation;
	gObj<MultiScatteringMarch> raymarch;
	gObj<Wavelets> wavelets;
	gObj<Filtering> filtering;

protected:

	int accSize = 512;

	int WAVELET_COUT = 4;

	gObj<Texture2D>* waveletOutputs;

	void Startup() {
		VolumeLoader::Startup();

		_ gLoad Pipeline(accumulation);
		_ gLoad Pipeline(raymarch);
		_ gLoad Pipeline(wavelets);
		_ gLoad Pipeline(filtering);

		accumulation->Accumulation = _ gCreate DrawableTexture3D<float>(accSize, accSize, accSize);
		accumulation->VolumeData = VolumeData;
		accumulation->Transforms = _ gCreate ConstantBuffer<float4x4>();

		raymarch->Radiance = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Positions = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Directions = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Gradient = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		raymarch->Camera = _ gCreate ConstantBuffer<float4x4>();
		raymarch->VolumeInfo = _ gCreate ConstantBuffer<VolumeInfo>();
		raymarch->VolumeData = VolumeData;
		raymarch->LightData = accumulation->Accumulation;
		raymarch->Transforms = _ gCreate ConstantBuffer<float4x4>();
		raymarch->Lighting = _ gCreate ConstantBuffer<Lighting>();

		wavelets->Radiance = raymarch->Radiance;
		wavelets->Positions = raymarch->Positions;
		wavelets->Directions = raymarch->Directions;
		wavelets->Gradient = raymarch->Gradient;

		waveletOutputs = new gObj<Texture2D>[WAVELET_COUT];
		for (int i=0; i< WAVELET_COUT; i++)
			waveletOutputs[i] = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);

		filtering->Accumulation = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		filtering->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		filtering->Radiance = waveletOutputs[WAVELET_COUT - 1];
		filtering->Positions = raymarch->Positions;
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
		
		raymarch->PassCount = FrameIndex;

		compute gSet Pipeline(raymarch);
		compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));

		for (int i = 0; i < WAVELET_COUT; i++) {
			wavelets->Info = int2 (1 << i, FrameIndex);
			wavelets->Input = i == 0 ? raymarch->Radiance : waveletOutputs[i - 1];
			wavelets->Output = waveletOutputs[i];
			compute gSet Pipeline(wavelets);
			compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));
		}

		filtering->PassCount = FrameIndex;
		compute gSet Pipeline(filtering);
		compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));

		compute gCopy All(render_target, filtering->Output);

		FrameIndex++;
	}
};