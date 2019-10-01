#pragma once

namespace OmegaEngine
{

class RenderGraphDescriptor
{
public:
    
    struct Texture
    {
       uint32_t width;
       uint32_t height;
       TextureFormat format = TextureFormat::FLOAT32;
       InitalState = InitalState::Clear;
    };
    
    struct Buffer
    {
        
    };
    
private:
   
};

struct RenderTarget
{
    Util::String name;
    
    // the target descriptor
    Descriptor descr;
};

class RenderGraph
{
public:
    
    void setup();
    
    void compile();
    
    void execute();
    
private:
    
    // all of the render targets accociated with this graph
    std::vector<RenderTarget> renderTargets;
    
    // a list of all the render passes
    std::evctor<RenderGraphPass> renderPasses;
};

}
