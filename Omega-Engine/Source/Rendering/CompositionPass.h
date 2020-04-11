#pragma once

#include "RenderGraph/RenderGraphBuilder.h"
#include "Rendering/Renderer.h"
#include "utility/CString.h"

namespace VulkanAPI
{
class Swapchain;
class VkDriver;
class ShaderProgram;
}

namespace OmegaEngine
{

class CompositionPass : public RenderStageBase
{
public:
    CompositionPass(VulkanAPI::VkDriver& driver, RenderGraph& rGraph, Util::String id, VulkanAPI::Swapchain& swapchain);
    ~CompositionPass();

    bool init(VulkanAPI::ProgramManager* manager) override;
    void setupPass() override;

private:
    // points to the render graph associated with this pass
    VulkanAPI::VkDriver& driver;
    RenderGraph& rGraph;
    VulkanAPI::Swapchain& swapchain;
    
    VulkanAPI::ShaderProgram* prog = nullptr;
    
    ResourceHandle backBuffer;
};

} // namespace OmegaEngine
