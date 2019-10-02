#pragma once

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/Managers/CommandBufferManager.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"

#include "Rendering/RenderCommon.h"
#include "Rendering/RenderConfig.h"
#include "Rendering/RenderInterface.h"
#include "VulkanAPI/Shader.h"

#include <functional>
#include <vector>

namespace VulkanAPI
{
// forward declearions
class Interface;
class RenderPass;
class Swapchain;
class BufferManager;
class Queue;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class RenderInterface;
class PostProcessInterface;
class IblInterface;

class DeferredRenderer : public RendererBase
{

public:
	DeferredRenderer::DeferredRenderer();
	~DeferredRenderer();

	void init();

	// abstract override
	void render(std::unique_ptr<VulkanAPI::Interface>& vkInterface, SceneType sceneType,
	            std::unique_ptr<RenderQueue>& renderQueue) override;

	void createGbufferPass();
	void createDeferredPipeline(std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
	                            VulkanAPI::Swapchain& swapchain);
	void createDeferredPass();

	void renderDeferredPass(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer);

private:
	vk::Device device;
	vk::PhysicalDevice gpu;

	// Command buffer handles for all passes
	VulkanAPI::CmdBufferHandle shadowCmdBufferHandle;
	VulkanAPI::CmdBufferHandle deferredCmdBufferHandle;
	VulkanAPI::CmdBufferHandle forwardCmdBufferHandle;

	// IBL environment mapping handler
	IblInterface iblInterface;

	// the post-processing manager
	PostProcessInterface postProcessInterface;

	// keep a local copy of the render config
	RenderConfig renderConfig;

	RenderGraph rGraph;
};

}    // namespace OmegaEngine