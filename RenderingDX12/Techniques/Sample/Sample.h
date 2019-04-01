#pragma once

#include "..\Common\ConstantBuffers.h"

class SamplePipeline : public GraphicsPipelineBindings {
public:
	gObj<Texture2D> renderTarget;
	gObj<Texture2D> depthBuffer;

	gObj<Texture2D> Diffuse_Map = nullptr;
	gObj<Texture2D> Specular_Map = nullptr;
	gObj<Texture2D> Bump_Map = nullptr;
	gObj<Texture2D> Mask_Map = nullptr;
	gObj<Buffer> globals;
	gObj<Buffer> locals;

	gObj<Buffer> lighting;
	gObj<Buffer> materials;

protected:
	void Setup() {
		_ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\Sample\\Sample_VS.cso"));
		_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\Sample\\Sample_PS.cso"));
		_ gSet InputLayout(SCENE_VERTEX::Layout());
		_ gSet DepthTest();
		_ gSet DepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	}

	void Globals()
	{
		RTV(0, renderTarget);
		DBV(depthBuffer);

		Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);

		CBV(0, lighting, ShaderType_Pixel);
		CBV(0, globals, ShaderType_Vertex);
	}

	void Locals()
	{
		SRV(0, Diffuse_Map, ShaderType_Pixel);
		SRV(1, Specular_Map, ShaderType_Pixel);
		SRV(2, Bump_Map, ShaderType_Pixel);
		SRV(3, Mask_Map, ShaderType_Pixel);
		
		CBV(1, materials, ShaderType_Pixel);
		CBV(1, locals, ShaderType_Vertex);
	}
};

class SampleTechnique : public Technique, public IHasScene, public IHasBackcolor, public IHasCamera, public IHasLight {
public:
	gObj<Texture2D> depthBuffer;
	gObj<Texture2D>* textures;
	gObj<Buffer>* materials;
	gObj<Buffer>* transforms;
	gObj<SamplePipeline> pipeline;
	gObj<Buffer> vertexBuffer;
	gObj<Buffer> globalsCB;
	gObj<Buffer> lightingCB;

protected:
	void Startup() {
		// Load and setup pipeline resource
		_ gLoad Pipeline(pipeline);
		
		// Create depth buffer resource
		gBind(depthBuffer) _ gCreate DepthBuffer(render_target->Width, render_target->Height);
		
		// Create globals vs constant buffer
		gBind(globalsCB) _ gCreate ConstantBuffer<Globals>();
		gBind(lightingCB) _ gCreate ConstantBuffer<Lighting>();

		pipeline->depthBuffer = depthBuffer;
		pipeline->globals = globalsCB;
		pipeline->lighting = lightingCB;

		// Load all other resources that needs an engine to be updated...
		perform(CreatingAssets);
	}

	void CreatingAssets(CopyingManager* manager) {

		// loading scene textures
		textures = new gObj<Texture2D>[Scene->Textures().size()];
		for (int i = 0; i < Scene->Textures().size(); i++)
			manager gLoad FromData(textures[i], Scene->Textures()[i]);

		// load full vertex buffer of all scene geometries
		gBind(vertexBuffer) _ gCreate VertexBuffer<SCENE_VERTEX>(Scene->VerticesCount());
		manager gCopy PtrData(vertexBuffer, Scene->Vertices());

		materials = new gObj<Buffer>[Scene->ObjectsCount()];
		transforms = new gObj<Buffer>[Scene->ObjectsCount()];
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			gBind(materials[i]) _ gCreate ConstantBuffer<Materialing>();
			gBind(transforms[i]) _ gCreate ConstantBuffer<Locals>();
		}

		for (int i = 0; i < Scene->ObjectsCount(); i++) {
			SCENE_OBJECT object = Scene->Objects()[i];
			Materialing materialing = { };
			materialing.Diffuse = object.Material->Diffuse;
			materialing.Specular = object.Material->Specular;
			materialing.SpecularSharpness = object.Material->SpecularSharpness;
			materialing.Emissive = object.Material->Emissive;
			materialing.Texture_Mask = { object.Material->Diffuse_Map >= 0 ? 1 : 0, object.Material->Specular_Map >= 0 ? 1 : 0, object.Material->Bump_Map >= 0 ? 1 : 0, object.Material->Mask_Map >= 0 ? 1 : 0 };

			manager gCopy ValueData(materials[i], materialing);
		}

		for (int i = 0; i < Scene->ObjectsCount(); i++) {
			SCENE_OBJECT object = Scene->Objects()[i];

			manager gCopy PtrData(transforms[i], object.Transform);
		}
	}

	void Graphics(GraphicsManager* manager) {
		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		pipeline->renderTarget = render_target;

		manager gCopy ValueData(globalsCB, Globals{ proj, view });
		manager gCopy ValueData(lightingCB, Lighting{
					mul(float4(Light->Position, 1), view).getXYZHomogenized(), 0,
					Light->Intensity
				});

#pragma region Updating locals constant buffers
		/*for (int i = 0; i < scene->ObjectsLength; i++) {
			SCENE_OBJECT &object = scene->Objects[i];

			with(manager)
				copy DataTo(transforms[i], object.Transform);
		}*/
#pragma endregion

		manager gClear RT(render_target, float4(Backcolor, 1));
		manager gClear Depth(depthBuffer, 1);

		manager gSet Viewport(render_target->Width, render_target->Height);
		manager gSet Pipeline(pipeline);
		manager gSet VertexBuffer(vertexBuffer);

		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto object = Scene->Objects()[i];
			// Set object texture to the pipeline field.
			pipeline->Diffuse_Map = object.Material->Diffuse_Map == -1 ? nullptr : textures[object.Material->Diffuse_Map];
			pipeline->Specular_Map = object.Material->Specular_Map == -1 ? nullptr : textures[object.Material->Specular_Map];
			pipeline->Bump_Map = object.Material->Bump_Map == -1 ? nullptr : textures[object.Material->Bump_Map];
			pipeline->Mask_Map = object.Material->Mask_Map == -1 ? nullptr : textures[object.Material->Mask_Map];
			pipeline->locals = transforms[i];
			pipeline->materials = materials[i];

			manager gDispatch Triangles(object.vertexesCount, object.startVertex);
		}
	}

	void Frame() {
		perform_async(Graphics);
	}
};

