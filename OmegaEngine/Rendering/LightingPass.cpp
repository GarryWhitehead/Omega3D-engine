#include "LightingPass.h"

#include "utility/Logger.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/VkDriver.h"

#include "VulkanAPI/Compiler/ShaderParser.h"

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
    
	VulkanAPI::ProgramManager::ShaderHash key = { filename.c_str(), variantBits, nullptr };
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
		if (!prog->prepare(parser, context))
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
		cmdBuffer->bindPipeline(context.rpass, prog);

		// bind the descriptor
        cmdBuffer->bindDescriptors(prog, VulkanAPI::Pipeline::Type::Graphics);

		cmdBuffer->bindPushBlock(prog, &renderConfig.ibl);

		// render full screen quad to screen
		cmdBuffer->drawQuad();
	});
}

}    // namespace OmegaEngine
