#pragma once
#include "RendererBase.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/CommandBufferManager.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Descriptors.h"
#include "Rendering/RenderConfig.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderCommon.h"

#include <vector>
#include <functional>

namespace VulkanAPI
{
	// forward declearions
	class Interface;
	class RenderPass;
	class Swapchain;
	class BufferManager;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderInterface;
	class PostProcessInterface;
	

	class DeferredRenderer : public RendererBase
	{

	public:

		DeferredRenderer::DeferredRenderer(vk::Device& dev,
			vk::PhysicalDevice& physical,
			std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager,
			std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
			VulkanAPI::Swapchain& swapchain, RenderConfig& _renderConfig);
		~DeferredRenderer();

		// abstract override
		void render(std::unique_ptr<VulkanAPI::Interface>& vkInterface, SceneType sceneType, std::unique_ptr<RenderQueue>& renderQueue) override;

		void createGbufferPass();
		void createDeferredPass(std::unique_ptr<VulkanAPI::BufferManager>& bufferManager, VulkanAPI::Swapchain& swapchain);

		void renderDeferredPass(std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager, VulkanAPI::Swapchain& swapchain);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		// images - for the gbuffer pass
		std::array<VulkanAPI::Texture, 6> gBufferImages;
		VulkanAPI::Texture shadowImage;
		VulkanAPI::Texture forwardOffscreenImage;
		VulkanAPI::Texture forwardOffscreenDepthImage;
	
		// Command buffer handles for all passes
		VulkanAPI::CmdBufferHandle deferredCmdBufferHandle;
		VulkanAPI::CmdBufferHandle forwardCmdBufferHandle;
		VulkanAPI::CmdBufferHandle objectCmdBufferHandle;

		// for the deferred rendering pipeline
		ProgramState state;

		// the post-processing manager
		std::unique_ptr<PostProcessInterface> postProcessInterface;

		// the final render call
		std::unique_ptr<PresentationPass> presentPass;

		// keep a local copy of the render config
		RenderConfig renderConfig;

	};

}