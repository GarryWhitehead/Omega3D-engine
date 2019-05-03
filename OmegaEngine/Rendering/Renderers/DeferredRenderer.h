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

		DeferredRenderer(vk::Device& device, vk::PhysicalDevice& physical, std::unique_ptr<VulkanAPI::CommandBufferManager>& cmd_buffer_manager, RenderConfig _render_config);
		~DeferredRenderer();

		// abstract override
		void render(std::unique_ptr<VulkanAPI::Interface>& vk_interface, SceneType scene_type) override;

		void create_gbuffer_pass();
		void create_deferred_pass(std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager, VulkanAPI::Swapchain& swapchain);

		void render_deferred(std::unique_ptr<VulkanAPI::CommandBufferManager>& cmd_buffer_manager, VulkanAPI::Swapchain& swapchain);
		

		VulkanAPI::RenderPass& get_deferred_pass()
		{
			return state.renderpass;
		}

		uint32_t get_attach_count() const
		{
			return deferred_render_info.renderpass.get_attach_count();
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		VulkanAPI::CmdBufferHandle cmd_buffer_handle;

		// for the gbuffer pass
		std::array<VulkanAPI::Texture, 6> gbuffer_images;
		
		// for the deferred rendering pipeline
		ProgramState state;

		// the post-processing manager
		std::unique_ptr<PostProcessInterface> pp_interface;

		// queued visible renderables
		std::unique_ptr<RenderQueue> render_queue;

		// keep a local copy of the render config
		RenderConfig render_config;

	};

}