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
	class Swapchain;
	class Interface;
}

namespace OmegaEngine
{
	// forward declerations
	class RenderQueue;
	struct RenderConfig;
	
	struct ProgramState
	{
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pipelineLayout;
		VulkanAPI::Pipeline pipeline;
		VulkanAPI::DescriptorLayout descriptorLayout;
		VulkanAPI::DescriptorSet descriptorSet;

		// information extracted from shader reflection
		VulkanAPI::BufferReflection bufferLayout;
		VulkanAPI::ImageReflection imageLayout;
	};

	namespace Rendering
	{
		void renderObjects(std::unique_ptr<RenderQueue>& renderQueue,
			VulkanAPI::RenderPass& renderpass,
			std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer,
			QueueType type,
			RenderConfig& renderConfig,
			bool clearAttachment);
	}

	class PresentationPass
	{
	public:

		PresentationPass(RenderConfig& renderConfig);
		~PresentationPass();

		void createPipeline(vk::ImageView& postProcessImageView, VulkanAPI::Interface& interface);
		void render(VulkanAPI::Interface& interface, RenderConfig& renderConfig);

	private:

		ProgramState state;
	};
}

