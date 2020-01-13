// DXR pipeline for photon tracing stage
struct DXR_PT_Pipeline : public RTPipelineManager {
	// Inherited via RTPipelineManager

	gObj<RayGenerationHandle> PTMainRays;
	gObj<MissHandle> PhotonMiss;
	gObj<ClosestHitHandle> PhotonScattering;
	gObj<HitGroupHandle> PhotonMaterial;

	class DXR_PT_IL : public DXIL_Library<DXR_PT_Pipeline> {
		void Setup() {
			_ gLoad DXIL(ShaderLoader::FromFile(".\\Techniques\\PhotonMaps\\PhotonTracing_RT.cso"));

			_ gLoad Shader(Context()->PTMainRays, L"PTMainRays");
			_ gLoad Shader(Context()->PhotonMiss, L"PhotonMiss");
			_ gLoad Shader(Context()->PhotonScattering, L"PhotonScattering");
		}
	};
	gObj<DXR_PT_IL> _Library;

	struct DXR_PT_Program : public RTProgram<DXR_PT_Pipeline> {
		void Setup() {
#ifdef COMPACT_PAYLOAD
			_ gSet Payload(6); // intensity 1half, rgb and bounce 1i
#else
			_ gSet Payload(16); // accumulation 3f, bounce 1i
#endif
			_ gSet StackSize(PHOTON_TRACE_MAX_BOUNCES);
			_ gLoad Shader(Context()->PTMainRays);
			_ gLoad Shader(Context()->PhotonMiss);
			_ gCreate HitGroup(Context()->PhotonMaterial, Context()->PhotonScattering, nullptr, nullptr);
		}

		gObj<SceneOnGPU> Scene;
		gObj<Buffer> Vertices;
		gObj<Buffer> Materials;

		// GBuffer Information
		gObj<Texture2D> Positions;
		gObj<Texture2D> Normals;
		gObj<Texture2D> Coordinates;
		gObj<Texture2D> MaterialIndices;

		gObj<Texture2D>* Textures;
		int TextureCount;

		gObj<Buffer> CameraCB;
		gObj<Buffer> LightingCB;
		int ProgressivePass;

		// Photon map binding objects
		gObj<Buffer> Photons; // Photon map in a lineal buffer
		gObj<Buffer> RadiusFactors; // Photon radius factor

		struct ObjInfo {
			int TriangleOffset;
			int MaterialIndex;
		} CurrentObjectInfo;

		void Globals() {
			UAV(0, Photons);
			UAV(1, RadiusFactors);

			ADS(0, Scene);
			SRV(1, Vertices);
			SRV(2, Materials);

			SRV(3, Positions);
			SRV(4, Normals);
			SRV(5, Coordinates);
			SRV(6, MaterialIndices);

			SRV_Array(7, Textures, TextureCount);

			Static_SMP(0, Sampler::Linear());

			CBV(0, CameraCB);
			CBV(1, LightingCB);
			CBV(2, ProgressivePass);
		}

		void HitGroup_Locals() {
			CBV(3, CurrentObjectInfo);
		}
	};
	gObj<DXR_PT_Program> _Program;

	void Setup() override
	{
		_ gLoad Library(_Library);
		_ gLoad Program(_Program);
	}
};