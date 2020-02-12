#pragma once
#include "../CA4G/ca4G.h"
#include "../GUI_Traits.h"
#include "../../stdafx.h"

class PhotonMapVolumeTechnique : public VolumeLoader, public IHasCamera, public IHasLight
{
	struct Accumulation : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\PerspectiveAccumulation_CS.cso"));
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

	struct PhotonTracing : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\VolumePhotonTracing_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Buffer> Photons;

		gObj<Buffer> VolumeInfo;
		gObj<Buffer> Lighting;
		int PassCount;

		void Globals() {
			UAV(0, Photons, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);

			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Any);

			CBV(0, VolumeInfo, ShaderType_Any);
			CBV(1, Lighting, ShaderType_Any);
		}

		void Locals() {
			CBV(2, PassCount, ShaderType_Any);
		}
	};

	struct PhotonSplatting : public GraphicsPipelineBindings {
		void Setup() {
			_ gSet VertexShader		(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\VolPhotonSplat_VS.cso"));
			_ gSet GeometryShader	(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\VolPhotonSplat_GS.cso"));
			_ gSet PixelShader		(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\VolPhotonSplat_PS.cso"));
			_ gSet AllRenderTargets(1, DXGI_FORMAT_R32G32B32A32_FLOAT);
			_ gSet BlendForAllRenderTargets(true, D3D12_BLEND_OP_ADD, D3D12_BLEND_ONE, D3D12_BLEND_ONE);
			_ gSet InputLayout({}); // null input layout to provide indices to vertex shader
		}

		gObj<Buffer> Photons;
		gObj<Buffer> CameraCB;
		gObj<Buffer> Transform;
		gObj<Buffer> VolumeInfo;
		gObj<Texture3D> Accumulation;
		gObj<Texture3D> DensityField;
		
		gObj<Texture2D> RenderTarget;

		void Globals() {
			RTV(0, RenderTarget);

			// Pixel bindings
			SRV(0, Photons, ShaderType_Pixel);
			SRV(1, Accumulation, ShaderType_Pixel);
			SRV(2, DensityField, ShaderType_Pixel);
			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Pixel);
			CBV(0, Transform, ShaderType_Pixel);
			CBV(1, VolumeInfo, ShaderType_Pixel);

			// Geometry bindings
			SRV(0, Photons, ShaderType_Geometry);
			CBV(0, CameraCB, ShaderType_Geometry);
		}
	};

	struct Pathtracing : public ComputePipelineBindings {
		void Setup() {
			_ gSet ComputeShader(ShaderLoader::FromFile(".\\Techniques\\VolumeRendering\\Combining_CS.cso"));
		}

		gObj<Texture3D> VolumeData;
		gObj<Texture2D> DirectLighting;

		gObj<Buffer> CameraCB;
		gObj<Buffer> VolumeInfo;
		int PassCount;

		gObj<Texture2D> Accumulation;
		gObj<Texture2D> Output;

		void Globals() {
			UAV(0, Accumulation, ShaderType_Any);
			UAV(1, Output, ShaderType_Any);

			SRV(0, VolumeData, ShaderType_Any);
			SRV(1, DirectLighting, ShaderType_Any);

			Static_SMP(0, Sampler::LinearWithoutMipMaps(), ShaderType_Any);

			CBV(0, CameraCB, ShaderType_Any);
			CBV(1, VolumeInfo, ShaderType_Any);
		}

		void Locals() {
			CBV(2, PassCount, ShaderType_Any);
		}
	};

	struct Photon {
		float3 Position;
		float3 Direction;
		float3 Intensity;
	};

	gObj<Accumulation> accumulation;
	gObj<PhotonTracing> tracing;
	gObj<PhotonSplatting> splatting;
	gObj<Pathtracing> pathtracing;

protected:

	int accSize = 512;

	void Startup() {
		VolumeLoader::Startup();

		_ gLoad Pipeline(accumulation);
		_ gLoad Pipeline(tracing);
		_ gLoad Pipeline(splatting);
		_ gLoad Pipeline(pathtracing);

		accumulation->Accumulation = _ gCreate DrawableTexture3D<float>(accSize, accSize, accSize);
		accumulation->VolumeData = VolumeData;
		accumulation->Transforms = _ gCreate ConstantBuffer<Globals>();

		tracing->Photons = _ gCreate RWStructuredBuffer<Photon>(PHOTON_DIMENSION * PHOTON_DIMENSION);
		tracing->VolumeInfo = _ gCreate ConstantBuffer<VolumeInfo>();
		tracing->VolumeData = VolumeData;
		tracing->Lighting = _ gCreate ConstantBuffer<Lighting>();

		splatting->Photons = tracing->Photons;
		splatting->CameraCB = _ gCreate ConstantBuffer<Globals>();
		splatting->RenderTarget = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		splatting->VolumeInfo = tracing->VolumeInfo;
		splatting->Transform = _ gCreate ConstantBuffer<Globals>();
		splatting->Accumulation = accumulation->Accumulation;
		splatting->DensityField = VolumeData;

		pathtracing->CameraCB = _ gCreate ConstantBuffer<float4x4>();
		pathtracing->Accumulation = _ gCreate DrawableTexture2D<float4>(render_target->Width, render_target->Height);
		pathtracing->Output = _ gCreate DrawableTexture2D<RGBA>(render_target->Width, render_target->Height);
		pathtracing->VolumeData = VolumeData;
		pathtracing->DirectLighting = splatting->RenderTarget;
		pathtracing->VolumeInfo = tracing->VolumeInfo;
	}

	void Frame() {
		if (CameraIsDirty)
			perform(Accumulating);

		perform(PhotonTrace);

		perform(PhotonSplat);

		perform(FinalPathtrace);
	}

	void UpdateFovAndPlanes(float3 viewer, float3 V, float3 corner, float& fov, float& zNear, float& zFar) {
		float3 C = corner - viewer;
		zNear = min(zNear, dot(V, C));
		zFar = max(zFar, dot(V, C));
		//if (dot(V, normalize(C)) > 0)
		fov = max(fov, acosf(dot(V, normalize(C))));
	}

	void GetFrustumWrapper(float3 viewerPosition, float3 corner, float4x4& view, float4x4& proj, float4x4& invProj)
	{
		float zNear = length(viewerPosition);
		float zFar = zNear;
		float3 V = -1 * viewerPosition / zNear; // normalize and invert viewerPosition
		float fov = 0;

		UpdateFovAndPlanes(viewerPosition, V, float3(corner.x, corner.y, corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(-corner.x, corner.y, corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(corner.x, -corner.y, corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(-corner.x, -corner.y, corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(corner.x, corner.y, -corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(-corner.x, corner.y, -corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(corner.x, -corner.y, -corner.z), fov, zNear, zFar);
		UpdateFovAndPlanes(viewerPosition, V, float3(-corner.x, -corner.y, -corner.z), fov, zNear, zFar);

		fov = min(fov, PI / 4);

		zNear = max(0.01, zNear);
		zFar = max(zFar, zNear + 0.01);

		view = LookAtLH(viewerPosition, float3(0, 0, 0), float3(0, 1, 0));
		proj = float4x4(
			1 / tanf(fov), 0, 0, 0,
			0, 1 / tanf(fov), 0, 0,
			0, 0, 1 / (zFar - zNear), 1,
			0, 0, -zNear / (zFar - zNear), 0
		);

		invProj = float4x4(
			tanf(fov), 0, 0, 0,
			0, tanf(fov), 0, 0,
			0, 0, (zFar - zNear), (zFar - zNear),
			0, 0, zNear, zNear
		);
	}

	float4x4 WorldToAccView;
	float4x4 AccProj;
	float4x4 AccProjInv;
	float4x4 WorldToVol;

	void Accumulating(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();
		float3 corner = float3(VolumeWidth, VolumeHeight, VolumeSlices) / max(VolumeWidth, max(VolumeHeight, VolumeSlices));

		GetFrustumWrapper(Camera->Position, corner, WorldToAccView, AccProj, AccProjInv);
		compute gCopy ValueData(accumulation->Transforms, Globals{
			AccProjInv,
			WorldToAccView.getInverse()
			});
		compute gSet Pipeline(accumulation);
		compute gDispatch Threads(accSize / CS_2D_GROUPSIZE, accSize / CS_2D_GROUPSIZE);
	}

	void PhotonTrace(gObj<GraphicsManager> manager) {
		auto compute = manager.Dynamic_Cast<ComputeManager>();

		static int FrameIndex = 0;

		if (LightSourceIsDirty || CameraIsDirty)
		{
			FrameIndex = 0;
			compute gCopy ValueData(tracing->VolumeInfo, VolumeInfo{
					VolumeWidth,
					VolumeHeight,
					VolumeSlices,
					densityScale,
					globalAbsortion
				});

			compute gCopy ValueData(tracing->Lighting,
				Lighting{
					Light->Position, 0,
					Light->Intensity, 0,
					Light->Direction, 0
				});
		}

		tracing->PassCount = FrameIndex;

		compute gSet Pipeline(tracing);
		compute gDispatch Threads(PHOTON_DIMENSION * PHOTON_DIMENSION / CS_1D_GROUPSIZE);

		FrameIndex++;
	}

	void PhotonSplat(gObj<GraphicsManager> manager) {

		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

 		manager gCopy ValueData(splatting->CameraCB, Globals{
			proj, 
			view
			});

		manager gCopy ValueData(splatting->Transform, Globals{
			AccProj,
			WorldToAccView
			});

		manager gClear RT(splatting->RenderTarget, float4(0, 0, 0, 0));

		manager gSet Pipeline(splatting);
		manager gSet Viewport(render_target->Width, render_target->Height);
		manager gDispatch Primitive(D3D_PRIMITIVE_TOPOLOGY_POINTLIST, PHOTON_DIMENSION * PHOTON_DIMENSION);
	}

	void FinalPathtrace(gObj<GraphicsManager> manager) {

		static int FrameIndex = 0;
		
		auto compute = manager.Dynamic_Cast<ComputeManager>();

		if (CameraIsDirty || LightSourceIsDirty) {

			FrameIndex = 0;

			float4x4 proj, view;
			Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
			compute gCopy ValueData(pathtracing->CameraCB, mul(proj.getInverse(), view.getInverse()));
		}

		pathtracing->PassCount = FrameIndex++;

		compute gSet Pipeline(pathtracing);

		compute gDispatch Threads((int)ceil(render_target->Width / (float)CS_2D_GROUPSIZE), (int)ceil(render_target->Height / (float)CS_2D_GROUPSIZE));

		compute gCopy All(render_target, pathtracing->Output);
	}
};