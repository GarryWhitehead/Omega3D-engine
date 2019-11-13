#include "LightingPass.h"

#include "utility/Logger.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkContext.h"

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
	builder.addInputAttachment("position");
	builder.addInputAttachment("colour");
	builder.addInputAttachment("normal");
	builder.addInputAttachment("emissive");
	builder.addInputAttachment("pbr");

	// create the gbuffer textures
	passInfo.output =
	    builder.createTexture({ .width = 2048, .height = 2048, .format = vk::Format::eR16G16B16A16Sfloat });

	// create the output taragets
	passInfo.output = builder.addOutputAttachment("lighting", passInfo.output);

	builder.addExecute([=](RGraphContext& rgContext, RenderContext& rContext) {
		// viewport and scissor
		context.cmdBuffer->setViewport();
		context.cmdBuffer->setScissor();

		// bind the pipeline
		VulkanAPI::Pipeline* pline = cbManager->findOrCreatePipeline(program, context.pass);
		context.cmdBuffer->bindPipeline(pline);

		// bind the descriptor
		VulkanAPI::DescriptorSet descrSet = cbManager->findOrCreateDescrSet(program);
		context.cmdBuffer->bindDescriptors(descrSet, VulkanAPI::PipelineType::Graphics);

		context.cmdBuffer->bindPushBlock(program, &renderConfig.ibl);

		// render full screen quad to screen
		context.cmdBuffer->drawQuad();
	});
}

}    // namespace OmegaEngine
