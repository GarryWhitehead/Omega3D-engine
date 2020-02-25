#include "LightingPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

LightingPass::LightingPass(RenderGraph& rGraph, Util::String id)
    : RenderStageBase(id.c_str()), rGraph(rGraph)
{
}

bool LightingPass::prepare(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "lighting.glsl";
    VulkanAPI::ProgramManager::ShaderHash key = {filename.c_str(), 0};
    VulkanAPI::ShaderProgram* prog = manager->getVariant(key);
    if (!prog)
    {
        return false;    
    }

    // build the lighting render pass
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    // read from the gbuffer targets
    builder.addReader("position");
    builder.addReader("colour");
    builder.addReader("normal");
    builder.addReader("pbr");
    builder.addReader("emissive");

    // create the gbuffer textures
    passInfo.output = builder.createRenderTarget(2048, 2048, vk::Format::eR16G16B16A16Sfloat);

    // create the output taragets
    passInfo.output = builder.addWriter("lighting", passInfo.output);

    builder.addExecute([=](RGraphContext& context) {

        // bind the pipeline
        VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
        context.cmdBuffer->bindPipeline(renderpass, prog);

        // bind the descriptor
        context.cmdBuffer->bindDescriptors(prog, VulkanAPI::Pipeline::Type::Graphics);
        // cmdBuffer->bindPushBlock(prog, &renderConfig.ibl);

        // render full screen quad to screen
        context.cmdBuffer->drawQuad();
    });

    return true;
}

} // namespace OmegaEngine
