#include "CompositionPass.h"

#include "VulkanAPI/Managers/ProgramManager.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

CompositionPass::CompositionPass(RenderGraph& rGraph, Util::String id)
    : rGraph(rGraph)
    , RenderStageBase(id)
{
}

CompositionPass::~CompositionPass()
{
}

bool CompositionPass::prepare(VulkanAPI::ShaderManager* manager)
{
	// load the shaders
	VulkanAPI::ShaderProgram* program = manager->findOrCreateShader("composition.glsl", nullptr, 0);
	if (!program)
	{
		LOGGER_ERROR("Fatal error whilst trying to compile shader for renderpass: %s.", passId);
		return false;
	}
}

}    // namespace OmegaEngine