#include "CompositionPass.h"

#include "VulkanAPI/ProgramManager.h"

#include "VulkanAPI/Compiler/ShaderParser.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

CompositionPass::CompositionPass(RenderGraph& rGraph, Util::String id)
    :   RenderStageBase(id.c_str())
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
    VulkanAPI::ProgramManager::ShaderHash key = { filename.c_str(), 0};
    VulkanAPI::ShaderProgram* prog = manager->getVariant(key);
    
    return true;
}

}    // namespace OmegaEngine
