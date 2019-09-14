#pragma once

#include "../../stdafx.h"

class APITConstructionPipeline : public GraphicsPipelineBindings {

};

class APITConstruction : public Technique, public IHasScene {
public:
    gObj<RetainedSceneLoader> sceneLoader;

protected:

};