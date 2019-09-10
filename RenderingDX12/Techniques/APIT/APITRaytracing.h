#include "../../stdafx.h"
#define RES 512


class APITRaytracing : public Technique {

    struct APITPipeline : public GraphicsPipelineBindings {
    public:
        gObj<Texture2D> renderTarget;
        gObj<Texture2D> depthComplexity;

    protected:
        void Setup() {
            
            _ gSet 
        }
    };
};