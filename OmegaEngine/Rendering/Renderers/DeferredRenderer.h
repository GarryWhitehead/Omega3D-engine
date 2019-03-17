#pragma once
#include "RendererBase.h"
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

	class DeferredRenderer : public RendererBase
	{

	public:

		DeferredRenderer(vk::Device device, vk::PhysicalDevice physical, RenderConfig _render_config);
		~DeferredRenderer();

		// abstract override
		void render(RenderInterface* rendeer_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface) override;

		void create_gbuffer_pass();
		void create_deferred_pass(uint32_t width, uint32_t height, std::unique_ptr<ComponentInterface>& component_interface);

		void render_deferred(VulkanAPI::Queue& graph_queue, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore);
		

		VulkanAPI::RenderPass& get_deferred_pass()
		{
			return renderpass;
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