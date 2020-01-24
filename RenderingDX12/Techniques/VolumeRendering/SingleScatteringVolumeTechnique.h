#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class SingleScatteringVolumeTechnique : public VolumeLoader, public IHasCamera, public IHasLight
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

	struct SingleScatteringMarch : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\SingleScattering_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Texture3D> LightData;
		gObj<Texture2D> Output;
		
		gObj<Buffer> Camera;
		gObj<Buffer> VolumeInfo;
		gObj<Buffer> Transforms;
		gObj<Buffer> Lighting;

		void Globals() {
			UAV(0, Output, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);
			SRV(1, LightData, ShaderType_Any);

			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Any);

			CBV(0, Camera, ShaderType_Any);
			CBV(1, VolumeInfo, ShaderType_Any);
			CBV(2, Transforms, ShaderType_Any);
			CBV(3, Lighting, ShaderType_Any);
		}
	};

	gObj<Accumulation> accumulation;
	gObj<SingleScatteringMarch> raymarch;

protected:

	int accSize =  512;

	void Startup() {
		VolumeLoader::Startup();

		_ gLoad Pipeline(accumulation);
		_ gLoad Pipeline(raymarch);

		accumulation->Accumulation = _ gCreate DrawableTexture3D<float>(accSize, accSize, accSize);
		accumulation->VolumeData = VolumeData;
		accumulation->Transforms = _ gCreate ConstantBuffer<float4x4>();

		raymarch->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		raymarch->Camera = _ gCreate ConstantBuffer<float4x4>();
		raymarch->VolumeInfo = _ gCreate ConstantBuffer<VolumeInfo>();
		raymarch->VolumeData = VolumeData;
		raymarch->LightData = accumulation->Accumulation;
		raymarch->Transforms = _ gCreate ConstantBuffer<float4x4>();
		raymarch->Lighting = _ gCreate ConstantBuffer<Lighting>();
	}

	void Frame() {
		perform(DrawingVolume);
	}

	void DrawingVolume(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

#pragma region Density Accumulation from Light
		
		if (LightSourceIsDirty) 
		{
			float3 lightDirection = normalize(Light->Direction);
			float3 corner = float3(VolumeWidth, VolumeHeight, VolumeSlices) / max(VolumeWidth, max(VolumeHeight, VolumeSlices));
			float size = length(corner);
			float3 accY = normalize(cross(
				dot(corner, lightDirection) < size ? corner : float3(corner.y, corner.z, corner.x),
				lightDirection)) * size;
			float3 accZ = lightDirection * size;
			float3 accX = normalize(cross(accY, accZ)) * size;

			// from accumulation to volume
			float4x4 transform = float4x4(
				accX.x, accX.y, accX.z, 0,
				accY.x, accY.y, accY.z, 0,
				accZ.x, accZ.y, accZ.z, 0,
				0, 0, 0, 1
			);
			transform = mul(
				mul(
					float4x4(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, -1, -1, -1, 1),
					transform),
				float4x4(corner.x, 0, 0, 0, 0, corner.y, 0, 0, 0, 0, corner.z, 0, 0.5, 0.5, 0.5, 1));
			
			compute gCopy ValueData(accumulation->Transforms, transform);
			compute gSet Pipeline(accumulation);
			compute gDispatch Threads(accSize / CS_2D_GROUPSIZE, accSize / CS_2D_GROUPSIZE);
			compute gCopy ValueData(raymarch->Transforms, transform.getInverse());
		}
#pragma endregion

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