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
	class Queue;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderInterface;
	class PostProcessInterface;
	class IblInterface;

	class DeferredRenderer : public RendererBase
	{

	public:

		enum class gBufferImageIndex
		{
			Position,
			BaseColour,
			Normal,
			Pbr,
			Emssive,
			Depth
		};

		// a lookup table to link the sampler name with the gbuffer image 
		std::vector<std::pair<std::string, gBufferImageIndex> > gbufferShaderLayout
		{
			{"positionSampler", gBufferImageIndex::Position},
			{"baseColourSampler", gBufferImageIndex::BaseColour},
			{"normalSampler", gBufferImageIndex::Normal},
			{"pbrSampler", gBufferImageIndex::Pbr},
			{"emissiveSampler", gBufferImageIndex::Emssive}
		};

		DeferredRenderer::DeferredRenderer(vk::Device& dev,
			vk::PhysicalDevice& physical,
			VulkanAPI::Queue& graphicsQueue,
			std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager,
			std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
			VulkanAPI::Swapchain& swapchain, RenderConfig& _renderConfig);
		~DeferredRenderer();

		// abstract override
		void render(std::unique_ptr<VulkanAPI::Interface>& vkInterface, SceneType sceneType, std::unique_ptr<RenderQueue>& renderQueue) override;

		void createGbufferPass();
		void createDeferredPipeline(std::unique_ptr<VulkanAPI::BufferManager>& bufferManager, VulkanAPI::Swapchain& swapchain);
		void createDeferredPass();

		void renderDeferredPass(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		// images - for the gbuffer pass
		std::array<VulkanAPI::Texture, 6> gBufferImages;
		VulkanAPI::Texture shadowImage;

		// deferred pass
		VulkanAPI::Texture deferredImage;
		VulkanAPI::RenderPass deferredRenderPass;
	
		// Command buffer handles for all passes
		VulkanAPI::CmdBufferHandle shadowCmdBufferHandle;
		VulkanAPI::CmdBufferHandle deferredCmdBufferHandle;
		VulkanAPI::CmdBufferHandle forwardCmdBufferHandle;

		// for the deferred rendering pipeline
		ProgramState state;

		// IBL environment mapping handler
		std::unique_ptr<IblInterface> iblInterface;

		// the post-processing manager
		std::unique_ptr<PostProcessInterface> postProcessInterface;

		// the final render call
		std::unique_ptr<PresentationPass> presentPass;

		// keep a local copy of the render config
		RenderConfig renderConfig;

	};

}