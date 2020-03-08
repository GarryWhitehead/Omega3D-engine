#include "CompositionPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

CompositionPass::CompositionPass(
    RenderGraph& rGraph, Util::String id, VulkanAPI::Swapchain& swapchain)
    : RenderStageBase(id.c_str()), rGraph(rGraph), swapchain(swapchain)
{
}

CompositionPass::~CompositionPass()
{
}

bool CompositionPass::prepare(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "composition.glsl";
    VulkanAPI::ProgramManager::ShaderKey key = {filename.c_str(), 0};
    VulkanAPI::ShaderProgram* prog = manager->getVariant(key);

    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    // final pass, write to the surface
    backBuffer =
        builder.importRenderTarget("surface", swapchain.getExtentsWidth(), swapchain.getExtentsHeight(), swapchain.getImageView(currentIndex));

    builder.addReader("skybox");
    builder.addWriter("composition", backBuffer);

    return true;
}

} // namespace OmegaEngine
