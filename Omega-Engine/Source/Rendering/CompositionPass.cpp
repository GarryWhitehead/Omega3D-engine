#include "CompositionPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

CompositionPass::CompositionPass(
    VulkanAPI::VkDriver& driver, RenderGraph& rGraph, Util::String id, VulkanAPI::Swapchain& swapchain)
    : RenderStageBase(id), driver(driver), rGraph(rGraph), swapchain(swapchain)
{
}

CompositionPass::~CompositionPass()
{
}

bool CompositionPass::init(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "composition.glsl";
    prog = manager->getVariantOrCreate(filename, 0);
    if (!prog)
    {
        return false;
    }
    return true;
}

void CompositionPass::setupPass()
{
    const Util::String passId = "compositionPass";
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);
    
    uint32_t currentImageIndex = driver.getCurrentImageIndex();
    
    // final pass, write to the surface
    // TODO: need to implement user defined sample size - set to one for now
    backBuffer =
        builder.importRenderTarget("surface", swapchain.getExtentsWidth(), swapchain.getExtentsHeight(), swapchain.getFormat(), 1, swapchain.getImageView(currentImageIndex));

    builder.addReader("skybox");
    builder.addWriter("composition", backBuffer);
    
    builder.addExecute([=](RGraphPassContext& rpassContext, RGraphContext& rgraphContext) {

        auto& cbManager = rgraphContext.driver->getCbManager();
        VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getScCommandBuffer(currentImageIndex);
        
        // bind the pipeline
        VulkanAPI::RenderPass* renderpass = rgraphContext.rGraph->getRenderpass(rpassContext.rpass);
        rgraphContext.driver->beginRenderpass(cmdBuffer, *renderpass);
        
        cmdBuffer->bindPipeline(cbManager, renderpass, prog, VulkanAPI::Pipeline::Type::Graphics);
        cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);
        
        // TODO: this needs to be user defined
        struct PushBlock
        {
            float expBias = 1.0f;
            float gamma = 0.2f;
        }pushBlock;
        
        cmdBuffer->bindPushBlock(prog, vk::ShaderStageFlagBits::eFragment, sizeof(PushBlock), &pushBlock);

        // render full screen quad to screen
        cmdBuffer->drawQuad();
        
        rgraphContext.driver->endRenderpass(cmdBuffer);
    });
}

} // namespace OmegaEngine
