#include "LightingPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CBufferManager.h"
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
    VulkanAPI::ShaderProgram* prog = manager->getVariantOrCreate(filename, 0);
    
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
    passInfo.output = builder.createRenderTarget("lighting_target", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);

    // create the output taragets
    passInfo.output = builder.addWriter("lighting", passInfo.output);

    builder.addExecute([=](RGraphContext& context) {

        auto& cbManager = context.driver->getCbManager();
        VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getCmdBuffer();
        // bind the pipeline
        VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
        cmdBuffer->bindPipeline(cbManager, renderpass, prog);

        // bind the descriptor
        cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);
        // cmdBuffer->bindPushBlock(prog, &renderConfig.ibl);

        // render full screen quad to screen
        cmdBuffer->drawQuad();
    });

    return true;
}

} // namespace OmegaEngine
