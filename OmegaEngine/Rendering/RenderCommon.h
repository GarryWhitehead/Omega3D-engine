#pragma once
#include "Rendering/ProgramStateManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/Datatypes/Texture.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Device.h"
#include "VulkanAPI/Interface.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"

namespace VulkanAPI
{
class RenderPass;
class Swapchain;
class Interface;
} // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class RenderQueue;
struct RenderConfig;

namespace Rendering
{
void renderObjects(std::unique_ptr<RenderQueue> &renderQueue, VulkanAPI::RenderPass &renderpass,
                   std::unique_ptr<VulkanAPI::CommandBuffer> &cmdBuffer, QueueType type,
                   RenderConfig &renderConfig, bool clearAttachment);
}

class PresentationPass
{
public:
	PresentationPass(RenderConfig &renderConfig);
	~PresentationPass();

	void createPipeline(vk::ImageView &postProcessImageView, VulkanAPI::Interface &interface);
	void render(VulkanAPI::Interface &interface, RenderConfig &renderConfig);

private:
	ProgramState state;
};
} // namespace OmegaEngine
