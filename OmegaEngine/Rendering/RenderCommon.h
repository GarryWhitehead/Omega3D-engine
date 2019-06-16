#pragma once

#include "VulkanAPI/Device.h"
#include "VulkanAPI/Datatypes/Texture.h"
#include "VulkanAPI/Interface.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"

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

