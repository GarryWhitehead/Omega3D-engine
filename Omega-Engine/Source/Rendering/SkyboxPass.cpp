#include "SkyboxPass.h"

#include "Types/Skybox.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Utility.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

SkyboxPass::SkyboxPass(RenderGraph& rGraph, Util::String id, OESkybox& skybox)
    :   RenderStageBase(id),
        rGraph(rGraph),
        skybox(skybox)
    
{
}

SkyboxPass::~SkyboxPass()
{
}

bool SkyboxPass::prepare(VulkanAPI::ProgramManager* manager)
{
	// load the shaders
    const Util::String filename = "SkyboxPass.glsl";
    VulkanAPI::ProgramManager::ShaderHash key { filename.c_str(), 0, nullptr };
    VulkanAPI::ShaderProgram* prog = manager->getVariant(key);

	RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

	//  use the output from the lighting pass as a input
	builder.addInputAttachment("lighting");
	builder.addOutputAttachment("SkyboxPass", output);
    
	// everything required to draw the skybox to the cmd buffer
    builder.addExecute([&](RGraphContext& context)
    {
        auto& cmdBuffer = context.cmdBuffer;
        
        cmdBuffer->bindPipeline(context.rpass, prog);
        
        cmdBuffer->bindDescriptors(prog, VulkanAPI::Pipeline::Type::Graphics);
        cmdBuffer->bindPushBlock(prog, vk::ShaderStageFlagBits::eFragment, sizeof(float), &skybox.blurFactor);

        cmdBuffer->bindVertexBuffer(skybox.vertexBuffer->get(), 0);
        cmdBuffer->bindIndexBuffer(skybox.indexBuffer->get(), 0);
        cmdBuffer->drawIndexed(OESkybox::indicesSize);
    });
    
    return true;
}

}    // namespace OmegaEngine
