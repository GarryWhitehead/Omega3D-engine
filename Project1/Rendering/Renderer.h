#pragma once
#include "Vulkan/RenderPass.h"
#include "Vulkan/Shader.h"
#include "Vulkan/PipelineInterface.h"
#include "Vulkan/DataTypes/Texture.h"

#include <vector>

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

		Renderer(RendererType type = RendererType::Deferred);
		~Renderer();

		void setup_deferred_renderer(uint32_t width, uint32_t height);

	private:

		RendererType type;

		struct RendererInfo
		{
			VulkanAPI::RenderPass renderpass;
			VulkanAPI::Shader shader;
			VulkanAPI::PipelineLayout pl_layout;
			VulkanAPI::Pipeline pipeline;
			std::array<VulkanAPI::Texture, (int)DeferredAttachments::Count> images;
		} renderer_info;
	};

}