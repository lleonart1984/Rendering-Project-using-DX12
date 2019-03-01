#pragma once

// Drawing the depth complexity of a scene using UAV 

#include "..\..\CA4G\ca4gScene.h"
#include "..\Common\ConstantBuffers.h"

// Notice this tutorial has two new traits, IHasScene and IHasLight.
// Those classes includes accessing to the loaded scene and the light source information
class Tutorial9 : public Technique, public IHasBackcolor, public IHasCamera, public IHasScene, public IHasLight {

	class DepthComplexityPipeline : public GraphicsPipelineBindings {
	public:
		// UAV to output the depth complexity
		gObj<Texture2D> depthComplexity;

		// Globals constant buffer with projection and view information for all scene
		gObj<Buffer> globals;
		// Locals constant buffer with world transform for each object
		gObj<Buffer> locals;

	protected:
		void Setup() {
			_ gSet VertexShader(LoadByteCode(".\\Techniques\\Tutorials\\Shaders\\TransformToViewSpace_VS.cso"));
			_ gSet PixelShader(LoadByteCode(".\\Techniques\\Tutorials\\Shaders\\GrabDepthComplexity_PS.cso"));
			_ gSet InputLayout(SCENE_VERTEX::Layout());
		}

		void Globals()
		{
			UAV(0, depthComplexity, ShaderType_Pixel);
			CBV(0, globals, ShaderType_Vertex);
		}

		void Locals()
		{
			CBV(1, locals, ShaderType_Vertex);
		}
	};

	
public:
	gObj<Texture2D> depthComplexity;
	gObj<Buffer>* transforms;
	gObj<Buffer> vertexBuffer;
	gObj<Buffer> globalsCB;
	gObj<Buffer> screenVertices;
	
	gObj<DepthComplexityPipeline> pipeline;
	gObj<ShowComplexityPipeline> showComplexity;

protected:
	void Startup() {
		// Load and setup pipeline resource
		_ gLoad Pipeline(pipeline);

		// Load pipeline to draw complexity
		_ gLoad Pipeline(showComplexity);

		// Create depth buffer resource
		depthComplexity = _ gCreate DrawableTexture2D<int>(render_target->Width, render_target->Height);

		// Create globals vs constant buffer
		globalsCB = _ gCreate ConstantBuffer<Globals>();

		pipeline->depthComplexity = depthComplexity;
		pipeline->globals = globalsCB;

		// Load all other resources that needs an engine to be updated...
		perform(CreatingAssets);
	}

	void CreatingAssets(CopyingManager* manager) {

		// load full vertex buffer of all scene geometries
		vertexBuffer = _ gCreate VertexBuffer<SCENE_VERTEX>(Scene->VerticesCount());
		manager gCopy PtrData(vertexBuffer, Scene->Vertices());

		// load screen vertices
		screenVertices = _ gCreate VertexBuffer<float2>(6);
		manager gCopy ListData(screenVertices, {
				float2(-1, 1),
				float2(1, 1),
				float2(1, -1),

				float2(1, -1),
				float2(-1, -1),
				float2(-1, 1),
			});

		transforms = new gObj<Buffer>[Scene->ObjectsCount()];
		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			transforms[i] = _ gCreate ConstantBuffer<Locals>();
		}

		for (int i = 0; i < Scene->ObjectsCount(); i++) {
			SCENE_OBJECT object = Scene->Objects()[i];

			manager gCopy PtrData(transforms[i], object.Transform);
		}
	}

	void Graphics(GraphicsManager* manager) {
		float4x4 view, proj;
		Camera->GetMatrices(render_target->Width, render_target->Height, view, proj);

		manager gCopy ValueData(globalsCB, Globals{ proj, view });

#pragma region Updating locals constant buffers
		/*for (int i = 0; i < scene->ObjectsLength; i++) {
			SCENE_OBJECT &object = scene->Objects[i];

			with(manager)
				copy DataTo(transforms[i], object.Transform);
		}*/
#pragma endregion

		manager gClear UAV(depthComplexity, 0U);
		manager gSet Viewport(render_target->Width, render_target->Height);
		manager gSet Pipeline(pipeline);
		manager gSet VertexBuffer(vertexBuffer);

		for (int i = 0; i < Scene->ObjectsCount(); i++)
		{
			auto object = Scene->Objects()[i];
			pipeline->locals = transforms[i];
			manager gDraw Triangles(object.vertexesCount, object.startVertex);
		}

		showComplexity->RenderTarget = render_target;
		showComplexity->complexity = depthComplexity;
		manager gClear RT(render_target, Backcolor);
		manager gSet Pipeline(showComplexity);
		manager gSet VertexBuffer(screenVertices);
		manager gDraw Triangles(6);
	}

	void Frame() {
		perform(Graphics);
	}
};