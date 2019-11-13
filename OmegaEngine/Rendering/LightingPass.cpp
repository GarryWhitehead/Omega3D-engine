#include "LightingPass.h"

#include "utility/Logger.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/Pipeline.h"

namespace OmegaEngine
{

LightingPass::LightingPass(RenderGraph& rGraph, Util::String id)
    : rGraph(rGraph)
    , RenderStageBase(id.c_str())
{
}

bool LightingPass::prepare(VulkanAPI::ShaderManager* manager)
{
    // load the shaders
    VulkanAPI::ShaderProgram* program = manager->findOrCreateShader("renderer/deferred/lighting.glsl", nullptr, 0);
	if (!program)
    {
		LOGGER_ERROR("Fatal error whilst trying to compile shader for renderpass: %s.", passId);
		return false;
	}

	RenderGraphBuilder builder = rGraph.createRenderPass(passId);
    
	// inputs from the deferred pass

	// create the gbuffer textures
	passInfo.output =
	    builder.createTexture("lighting", { .width = 2048, .height = 2048, .format = vk::Format::eR16G16B16A16Sfloat });

	// create the output taragets
	passInfo.output = builder.addOutput(passInfo.output);
    
	builder.addExecute([program, cbManager](RenderInfo& rInfo, RGraphContext& context) {
		// viewport and scissor
		context.cmdBuffer->setViewport();
		context.cmdBuffer->setScissor();

		// bind the pipeline
        VulkanAPI::Pipeline* pline = cbManager->findOrCreatePipeline(handle, context.pass);
		context.cmdBuffer->bindPipeline(pline);
        
        // bind the descriptor
        VulkanAPI::DescriptorSet descrSet = cbManager->findOrCreateDescrSet(handle);
		context.cmdBuffer->bindDescriptors(rInfo.descriptorSet,
		                                   VulkanAPI::PipelineType::Graphics);

		context.cmdBuffer->bindPushBlock(rInfo.pipelineLayout, vk::ShaderStageFlagBits::eFragment,
		                                 sizeof(RenderConfig::IBLInfo), &renderConfig.ibl);

		// render full screen quad to screen
		context.cmdBuffer->drawQuad();

	});
}

}    // namespace OmegaEngine
