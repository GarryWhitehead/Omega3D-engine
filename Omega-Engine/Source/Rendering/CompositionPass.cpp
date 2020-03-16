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
    RenderGraph& rGraph, Util::String id, VulkanAPI::Swapchain& swapchain)
    : rGraph(rGraph), swapchain(swapchain)
{
}

CompositionPass::~CompositionPass()
{
}

bool CompositionPass::prepare(VulkanAPI::VkDriver& driver, VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "composition.glsl";
    VulkanAPI::ProgramManager::ShaderKey key = {filename.c_str(), 0};
    VulkanAPI::ShaderProgram* prog = manager->getVariant(key);

    Util::String passId = "compositionPass";
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);
    
    uint32_t currentImageIndex = driver.getCurrentImageIndex();
    
    // final pass, write to the surface
    backBuffer =
        builder.importRenderTarget("surface", swapchain.getExtentsWidth(), swapchain.getExtentsHeight(), swapchain.getImageView(currentImageIndex));

    builder.addReader("skybox");
    builder.addWriter("composition", backBuffer);

    return true;
}

} // namespace OmegaEngine
