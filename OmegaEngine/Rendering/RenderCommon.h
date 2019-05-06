#pragma once

#include "Vulkan/Device.h"
#include "Vulkan/Datatypes/Texture.h"
#include "Vulkan/Interface.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/CommandBufferManager.h"

namespace VulkanAPI
{
	class RenderPass;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderQueue;
	struct RenderConfig;
	
	struct ProgramState
	{
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pl_layout;
		VulkanAPI::Pipeline pipeline;
		VulkanAPI::DescriptorLayout descr_layout;
		VulkanAPI::DescriptorSet descr_set;

		// information extracted from shader reflection
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		VulkanAPI::ImageLayoutBuffer image_layout;
	};

	namespace Rendering
	{
		void render_objects(std::unique_ptr<RenderQueue>& render_queue,
			VulkanAPI::RenderPass& renderpass,
			std::unique_ptr<VulkanAPI::CommandBuffer>& cmd_buffer,
			QueueType type,
			RenderConfig& render_config);
	}
}

