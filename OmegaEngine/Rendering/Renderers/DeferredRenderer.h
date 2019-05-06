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
			std::unique_ptr<VulkanAPI::CommandBufferManager>& cmd_buffer_manager,
			std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
			VulkanAPI::Swapchain& swapchain, RenderConfig& _render_config);
		~DeferredRenderer();

		// abstract override
		void render(std::unique_ptr<VulkanAPI::Interface>& vk_interface, SceneType scene_type, std::unique_ptr<RenderQueue>& render_queue) override;

		void create_gbuffer_pass();
		void create_shadow_pass();
		void create_deferred_pass(std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager, VulkanAPI::Swapchain& swapchain);

		void render_deferred(std::unique_ptr<VulkanAPI::CommandBufferManager>& cmd_buffer_manager, VulkanAPI::Swapchain& swapchain);
		

		VulkanAPI::RenderPass& get_deferred_pass()
		{
			return deferred_pass;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		VulkanAPI::CmdBufferHandle cmd_buffer_handle;

		// images - for the gbuffer pass
		std::array<VulkanAPI::Texture, 6> gbuffer_images;
		VulkanAPI::Texture shadow_image;
		VulkanAPI::Texture deferred_offscreen_image;
		VulkanAPI::Texture deferred_offscreen_depth_image;
		
		// Renderpasses
		VulkanAPI::RenderPass deferred_pass;
		
		// Command buffer hanldes
		VulkanAPI::CmdBufferHandle forward_cmd_buffer_handle;
		VulkanAPI::CmdBufferHandle obj_cmd_buffer_handle;

		// for the deferred rendering pipeline
		ProgramState state;

		// the post-processing manager
		std::unique_ptr<PostProcessInterface> pp_interface;

		// keep a local copy of the render config
		RenderConfig render_config;

	};

}