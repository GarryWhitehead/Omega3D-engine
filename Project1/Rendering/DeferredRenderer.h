#pragma once
#include "Vulkan/Shader.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/CommandBuffer.h"
#include "OEMaths/OEMaths.h"

#include <vector>

namespace VulkanAPI
{
	// forward declearions
	class Sampler;
	class Pipeline;
	class DescriptorLayout;
	class DescriptorSet;
	class Interface;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderInterface;
	class PostProcessInterface;

	enum class DeferredAttachments
	{
		Position,
		Albedo,
		Normal,
		Bump,
		Occlusion,
		Metallic,
		Roughness,
		Depth,
		Count
	};

	class DeferredRenderer
	{

	public:

		DeferredRenderer(vk::Device device);
		~DeferredRenderer();

		void create(uint32_t width, uint32_t height);

		void render_deferred(VulkanAPI::Queue& graph_queue, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore);
		void render(RenderInterface* rendeer_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface);

		std::function<void()> set_render_callback(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface);

	private:

		vk::Device device;

		// for the rendering pipeline
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		std::unique_ptr<VulkanAPI::Pipeline> pipeline;
		std::unique_ptr<VulkanAPI::DescriptorLayout> descr_layout;
		std::unique_ptr<VulkanAPI::DescriptorSet> descr_set;
		std::unique_ptr<VulkanAPI::RenderPass> renderpass;

		VulkanAPI::Buffer vert_buffer;
		VulkanAPI::Buffer frag_buffer;

		std::unique_ptr<VulkanAPI::Sampler> linear_sampler;
		std::array<VulkanAPI::Texture, (int)DeferredAttachments::Count> images;

		VulkanAPI::CommandBuffer cmd_buffer;

		// the post-processing manager
		std::unique_ptr<PostProcessInterface> pp_interface;
	};

}