#pragma once

#include "RenderGraph/RenderGraphBuilder.h"
#include "Rendering/Renderer.h"
#include "utility/CString.h"

namespace VulkanAPI
{
class Swapchain;
class VkDriver;
}

namespace OmegaEngine
{

class CompositionPass
{
public:
    CompositionPass(RenderGraph& rGraph, Util::String id, VulkanAPI::Swapchain& swapchain);
    ~CompositionPass();

    bool prepare(VulkanAPI::VkDriver& driver, VulkanAPI::ProgramManager* manager);

private:
    // points to the render graph associated with this pass
    RenderGraph& rGraph;
    VulkanAPI::Swapchain& swapchain;

    ResourceHandle backBuffer;
};

} // namespace OmegaEngine
