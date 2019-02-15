#pragma once

// Creating a basic technique to clear the render target with some color

#include "..\..\CA4G\ca4GScene.h"

class Tutorial1 : 
	// All techniques should inherit from Technique class and may implement Startup (for initialization) and Frame (updating graphics every frame).
	public Technique, 
	// Inherit from IHasBackcolor to add a float3 backcolor field the application can provide.
	public IHasBackcolor {
	
	void Frame() {
		// Every frame perform a single process using a Graphics Engine
		// Perform just populates the sublaying command list with the commands of the process
		perform(GraphicProcess);
	}

	// Graphic Process to clear the render target with a backcolor
	void GraphicProcess(GraphicsManager *manager) {
		// the DSL is for legibility
		// render_target macro expands to getManager()->getBackBuffer(), returing the current frame render target being rendered.
		manager	gClear RT(render_target, Backcolor);
	}
};