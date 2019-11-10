#include "LightingPass.h"

#include "utility/Logger.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/Shader.h"

namespace OmegaEngine
{

bool LightingPass::prepare(RenderGraph& rGraph, VulkanAPI::ShaderManager* manager)
{
    // load the shaders
   std::string outputBuffer;
   if (!manager->load("renderer/deferred/lighting.glsl", outputBuffer))
   {
       LOGGER_ERROR("Unable to load deferred renderer shaders.");
       return false;
   }
   ShaderHandle handle = manager->parse(outputBuffer);
   if (handle == UINT32_MAX)
   {
       LOGGER_ERROR("Unable to parse shader.");
       return false;
   }
    
    RenderGraphBuilder builder = rGraph.createRenderPass("Lighting Pass");

	// inputs from the deferred pass


	// create the gbuffer textures
	passInfo.output =
	    builder.createTexture("lighting", { .width = 2048, .height = 2048, .format = vk::Format::eR16G16B16A16Sfloat });

	// create the output taragets
	passInfo.output = builder.addOutput(passInfo.output);

	builder.addExecute([](RenderingInfo& rInfo, RGraphContext& context)
	{
		vk::RenderPassBeginInfo beginInfo =
		    forwardRenderpass.getBeginInfo(vk::ClearColorValue(renderConfig.general.backgroundColour));
		context.cmdBuffer->beginRenderpass(beginInfo);

		// viewport and scissor
		context.cmdBuffer->setViewport();
		context.cmdBuffer->setScissor();

		// bind everything required to draw
		context.cmdBuffer->bindPipeline(rInfo.pipeline);
		context.cmdBuffer->bindDescriptors(rInfo.pipelineLayout, rInfo.descriptorSet,
		                                   VulkanAPI::PipelineType::Graphics);
		context.cmdBuffer->bindPushBlock(rInfo.pipelineLayout, vk::ShaderStageFlagBits::eFragment,
		                         sizeof(RenderConfig::IBLInfo), &renderConfig.ibl);

		// render full screen quad to screen
		context.cmdBuffer->drawQuad();

		// end this pass and cmd buffer
		context.cmdBuffer->endRenderpass();
	});
}

}    // namespace OmegaEngine
