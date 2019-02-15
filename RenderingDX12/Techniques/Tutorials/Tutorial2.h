#pragma once

// Creating a basic technique to clear the render target with some color
// In two different threads. Depending on the race condition will be shown
// one color or another

#include "..\..\CA4G\ca4GScene.h"

class Tutorial2 : public Technique, public IHasBackcolor {
	
	void Frame() {
		// Every frame perform a two process asynchronously using a Graphics Engine
		
		// Enqueue this process for an async execution
		perform_async(GraphicProcess1);

		// Enqueue this process for an async execution
		perform_async(GraphicProcess2);
		
		// Depending on what thread locks first the queue is the color
		// used to clear last the render target

		// Flush all execution to the GPU
		//flush_all_to_gpu;
	}

	// Graphic Process to clear the render target with a backcolor
	void GraphicProcess1(GraphicsManager *manager) {
		manager gClear RT(render_target, Backcolor); // Backcolor field was inherited from IHasBackcolor class
	}
	void GraphicProcess2(GraphicsManager *manager) {
		manager gClear RT(render_target, float3(1,1,1) - Backcolor);
	}
};