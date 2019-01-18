#pragma once
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Buffer.h"
#include "OEMaths/OEMaths.h"
#include <vector>

namespace VulkanAPI
{
	// forward declearions
	class Sampler;
	class RenderPass;
	class CommandBuffer;
}

namespace OmegaEngine
{
	
	// Note: only deferred renderer is supported at the moment. More to follow....
	enum class RendererType
	{
		Deferred,
		Tile_Based,
		Count
	};

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

	class Renderer
	{

	public:

		struct VertBuffer
		{
			OEMaths::mat4f mvp;
		};

		Renderer(vk::Device device, RendererType type = RendererType::Deferred);
		~Renderer();

		vk::RenderPass& get_renderpass()
		{
			return renderpass->get();
		}

		uint8_t get_attach_count() const
		{
			return renderpass->get_attach_count();
		}

		void create_deferred(vk::Device device, uint32_t width, uint32_t height);

		VulkanAPI::CommandBuffer& begin();
		void render();

	private:

		RendererType type;

		// for the rendering pipeline
		std::unique_ptr<VulkanAPI::RenderPass> renderpass;
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		VulkanAPI::Pipeline pipeline;
		VulkanAPI::DescriptorLayout descr_layout;
		VulkanAPI::DescriptorSet descr_set;

		VulkanAPI::Buffer vert_buffer;
		VulkanAPI::Buffer frag_buffer;

		std::unique_ptr<VulkanAPI::Sampler> linear_sampler;
		std::array<VulkanAPI::Texture, (int)DeferredAttachments::Count> images;

		// command buffer 
		std::unique_ptr<VulkanAPI::CommandBuffer> cmd_buffer;

	};

}