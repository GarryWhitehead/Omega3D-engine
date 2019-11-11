#include "LightingPass.h"

#include "utility/Logger.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkContext.h"

namespace OmegaEngine
{

LightingPass::LightingPass(RenderGraph& rGraph, VulkanAPI::ShaderManager& manager, Util::String id)
    : rGraph(rGraph)
    , RenderStageBase(manager, id)
{
}

bool LightingPass::create()
{
	// load the shaders
	std::string outputBuffer;
	if (!shaderMan.load("renderer/deferred/lighting.glsl", outputBuffer))
	{
		LOGGER_ERROR("Unable to load deferred renderer shaders.");
		return false;
	}
	handle = shaderMan.parse(outputBuffer);
	if (handle == UINT32_MAX)
	{
		LOGGER_ERROR("Unable to parse shader.");
		return false;
	}

	return true;
}

bool LightingPass::preparePass(RGraphContext& context)
{

	RenderGraphBuilder builder = rGraph.createRenderPass(passId);

	// inputs from the deferred pass


	// create the gbuffer textures
	passInfo.output =
	    builder.createTexture("lighting", { .width = 2048, .height = 2048, .format = vk::Format::eR16G16B16A16Sfloat });

	// create the output taragets
	passInfo.output = builder.addOutput(passInfo.output);

	builder.addExecute([&context](RenderInfo& rInfo) {

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

	});
}

}    // namespace OmegaEngine
