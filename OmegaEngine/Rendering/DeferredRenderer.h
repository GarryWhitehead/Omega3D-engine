#pragma once
#include "Vulkan/Shader.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/CommandBuffer.h"
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
}

namespace OmegaEngine
{
	// forward declerations
	class RenderInterface;
	class PostProcessInterface;
	class CameraManager;

	class DeferredRenderer
	{

	public:

		DeferredRenderer(vk::Device device, vk::PhysicalDevice physical, RenderConfig _render_config);
		~DeferredRenderer();

		void create_gbuffer_pass();
		void create_deferred_pass(uint32_t width, uint32_t height, CameraManager& camera_manager);

		void render_deferred(VulkanAPI::Queue& graph_queue, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore);
		void render(RenderInterface* rendeer_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface);

		std::function<void()> set_render_callback(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface);

		VulkanAPI::RenderPass& get_deferred_pass()
		{
			return renderpass;
		}

		VulkanAPI::RenderPass& get_gbuffer_pass()
		{
			return gbuffer_renderpass;
		}

		uint32_t get_attach_count() const
		{
			return renderpass.get_attach_count();
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		// for the gbuffer pass
		std::array<VulkanAPI::Texture, 6> gbuffer_images;
		VulkanAPI::RenderPass gbuffer_renderpass;

		// for the rendering pipeline
		VulkanAPI::Texture image;
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		VulkanAPI::Pipeline pipeline;
		VulkanAPI::DescriptorLayout descr_layout;
		VulkanAPI::DescriptorSet descr_set;
		VulkanAPI::RenderPass renderpass;

		VulkanAPI::Buffer vert_buffer;
		VulkanAPI::Buffer frag_buffer;

		VulkanAPI::CommandBuffer cmd_buffer;

		// the post-processing manager
		std::unique_ptr<PostProcessInterface> pp_interface;

		// keep a local copy of the render config
		RenderConfig render_config;
	};

}