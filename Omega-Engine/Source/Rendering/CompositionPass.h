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

class CompositionPass : public RenderStageBase
{
public:
    CompositionPass(VulkanAPI::VkDriver& driver, RenderGraph& rGraph, Util::String id, VulkanAPI::Swapchain& swapchain);
    ~CompositionPass();

    bool prepare(VulkanAPI::ProgramManager* manager) override;

private:
    // points to the render graph associated with this pass
    VulkanAPI::VkDriver& driver;
    RenderGraph& rGraph;
    VulkanAPI::Swapchain& swapchain;

    ResourceHandle backBuffer;
};

} // namespace OmegaEngine
