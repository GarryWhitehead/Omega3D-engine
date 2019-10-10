#pragma once

#include "RenderGraph/RenderGraph.h"

namespace OmegaEngine
{

class GBufferFillPass
{
public:
    
    struct GBufferInfo
    {
        ResourceHandle position;
        ResourceHandle colour;
        ResourceHandle normal;
        ResourceHandle emissive;
        ResourceHandle pbr;
        ResourceHandle depth;
    };
    
    GBufferFillPass() {}
    
    // no copying
    GBufferFillPass(const GBufferFillPass&) = delete;
    GBufferFillPass& operator=(const GBufferFillPass&) = delete;
    
    void init();
    
private:
    
    RenderGraph *rGraph = nullptr;
    
    GBufferInfo gbufferInfo;
    
};

}
