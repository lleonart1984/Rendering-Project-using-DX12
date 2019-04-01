#pragma once

#include "..\Common\RetainedSceneLoader.h"
#include "..\Common\ConstantBuffers.h"

class RetainedPipeline : public GraphicsPipelineBindings {
public:
	gObj<Texture2D> renderTarget;
	gObj<Texture2D> depthBuffer;

	// CBs
	gObj<Buffer> globals;
	gObj<Buffer> lighting;

	// SRVs
	gObj<Buffer> vertices;
	gObj<Buffer> objectIds;

	gObj<Buffer> transforms;
	gObj<Buffer> materialIndices;

	gObj<Buffer> materials;
	
	gObj<Texture2D>* Textures = nullptr;
	int TextureCount;

protected:
	void Setup() {
		_ gSet VertexShader(ShaderLoader::FromFile(".\\Techniques\\RetainedBasic\\RetainedRendering_VS.cso"));
		_ gSet PixelShader(ShaderLoader::FromFile(".\\Techniques\\RetainedBasic\\RetainedRendering_PS.cso"));
		_ gSet InputLayout({});
		_ gSet DepthTest();
	}

	void Globals()
	{
		RTV(0, renderTarget);
		DBV(depthBuffer);

		Static_SMP(0, Sampler::Linear(), ShaderType_Pixel);

		CBV(0, lighting, ShaderType_Pixel);
		SRV(0, materialIndices, ShaderType_Pixel);
		SRV(1, materials, ShaderType_Pixel);
		CBV(0, globals, ShaderType_Vertex);
		SRV(0, vertices, ShaderType_Vertex);
		SRV(1, objectIds, ShaderType_Vertex);
		SRV(2, transforms, ShaderType_Vertex);
		SRV_Array(2, Textures, TextureCount, ShaderType_Pixel);
	}
};


class RetainedBasicRenderer : public RetainedSceneLoader, public IHasCamera, public IHasLight, public IHasBackcolor {
public:
	gObj<Texture2D> depthBuffer;
	gObj<RetainedPipeline> pipeline;
	gObj<Buffer> globalsCB;
	gObj<Buffer> lightingCB;

protected:
	void Startup() {
		// Load scene in retained mode
		RetainedSceneLoader::Startup();

		// Load and setup pipeline resource
		_ gLoad Pipeline(pipeline);

		// Create depth buffer resource
		gBind(depthBuffer) _ gCreate DepthBuffer(render_target->Width, render_target->Height);

		// Create globals VS constant buffer
		gBind(globalsCB) _ gCreate ConstantBuffer<Globals>();
		gBind(lightingCB) _ gCreate ConstantBuffer<Lighting>();

		pipeline->depthBuffer = depthBuffer;
		pipeline->globals = globalsCB;
		pipeline->lighting = lightingCB;

		pipeline->TextureCount = TextureCount;
		pipeline->Textures = Textures;
		pipeline->vertices = VertexBuffer;
		pipeline->objectIds = ObjectBuffer;
		pipeline->materialIndices = MaterialIndexBuffer;
		pipeline->materials = MaterialBuffer;
		pipeline->transforms = TransformBuffer;
	}

	void Graphics(GraphicsManager* manager) {
		
		pipeline->renderTarget = render_target;

		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);
		manager gCopy ValueData(globalsCB, Globals{ proj, view });
		manager gCopy ValueData(lightingCB, Lighting{
					mul(float4(Light->Position, 1), view).getXYZHomogenized(), 0,
					Light->Intensity
				});

		manager gClear RT(render_target, float4(Backcolor, 1));
		manager gClear Depth(depthBuffer, 1);

		manager gSet Viewport(render_target->Width, render_target->Height);
		manager gSet Pipeline(pipeline);

		manager gDispatch Triangles(VertexBuffer->ElementCount);
	}

	void Frame() {
		perform(Graphics);
	}
};