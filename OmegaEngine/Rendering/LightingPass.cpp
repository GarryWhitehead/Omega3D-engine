#include "LightingPass.h"

#include "utility/Logger.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/VkDriver.h"

namespace OmegaEngine
{

LightingPass::LightingPass(RenderGraph& rGraph, Util::String id)
    : rGraph(rGraph)
    , RenderStageBase(id.c_str())
{
}

bool LightingPass::prepare(VulkanAPI::ProgramManager* manager)
{
	// load the shaders
	const Util::String filename = "lighting.glsl";
    VulkanAPI::ShaderProgram* prog = nullptr;
    
	if (!manager->hasShaderVariant(filename, nullptr, variantBits))
	{
		VulkanAPI::ShaderParser parser;
		if (!parser.parse(filename))
		{
			return false;
		}
        prog = manager->createNewInstance(filename, nullptr, variantBits);

		// add variants and constant values

		assert(prog);
		if (!prog->prepare(parser))
		{
			return false;
		}
	}

	// build the lighting render pass
	RenderGraphBuilder builder = rGraph.createRenderPass(passId);

	// inputs from the deferred pass
	builder.addInputAttachment("position");
	builder.addInputAttachment("colour");
	builder.addInputAttachment("normal");
	builder.addInputAttachment("emissive");
	builder.addInputAttachment("pbr");

	// create the gbuffer textures
	passInfo.output = builder.createTexture(2048, 2048, vk::Format::eR16G16B16A16Sfloat);

	// create the output taragets
	passInfo.output = builder.addOutputAttachment("lighting", passInfo.output);

	builder.addExecute([=](RGraphContext& context) {
        
        auto& cmdBuffer = context.cbManager->getCmdBuffer(context.cmdBuffer);
        
		// bind the pipeline
		VulkanAPI::Pipeline* pline = context.cbManager->findOrCreatePipeline(prog, context.rpass);
		cmdBuffer->bindPipeline(pline);

		// bind the descriptor
		VulkanAPI::DescriptorSet descrSet = context.cbManager->findOrCreateDescrSet(prog->descrLayout);
        cmdBuffer->bindDescriptors(descrSet, VulkanAPI::Pipeline::Type::Graphics);

		cmdBuffer->bindPushBlock(prog, &renderConfig.ibl);

		// render full screen quad to screen
		cmdBuffer->drawQuad();
	});
}

}    // namespace OmegaEngine
