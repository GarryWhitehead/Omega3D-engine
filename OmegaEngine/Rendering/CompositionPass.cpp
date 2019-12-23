#include "CompositionPass.h"

#include "VulkanAPI/ProgramManager.h"

#include "VulkanAPI/Compiler/ShaderParser.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

CompositionPass::CompositionPass(RenderGraph& rGraph, Util::String id)
    :   RenderStageBase(id)
      , rGraph(rGraph)
{
}

CompositionPass::~CompositionPass()
{
}

bool CompositionPass::prepare(VulkanAPI::ProgramManager* manager)
{
	// load the shaders
    const Util::String filename = "composition.glsl";
    VulkanAPI::ShaderProgram* prog = nullptr;
    
    VulkanAPI::ProgramManager::ShaderHash key = { filename.c_str(), 0, nullptr };
    if (!manager->hasShaderVariant(key))
    {
        VulkanAPI::ShaderParser parser;
        if (!parser.parse(filename))
        {
            return false;
        }
        prog = manager->createNewInstance(key);

        // add variants and constant values

        assert(prog);
        if (!prog->prepare(parser))
        {
            return false;
        }
    }
    
    return true;
}

}    // namespace OmegaEngine
