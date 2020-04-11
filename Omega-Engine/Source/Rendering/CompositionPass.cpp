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
    backBuffer =
        builder.importRenderTarget("surface", swapchain.getExtentsWidth(), swapchain.getExtentsHeight(), swapchain.getImageView(currentImageIndex));

    builder.addReader("skybox");
    builder.addWriter("composition", backBuffer);

}

} // namespace OmegaEngine
