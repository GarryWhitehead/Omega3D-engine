#include "RenderCommon.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Skybox.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/BufferManager.h"
#include "Managers/EventManager.h"
#include "Engine/Omega_Global.h"

namespace OmegaEngine
{

	namespace Rendering
	{
		void renderObjects(std::unique_ptr<RenderQueue>& renderQueue,
			VulkanAPI::RenderPass& renderpass,
			std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer,
			QueueType type,
			RenderConfig& renderConfig,
			bool clearAttachment)
		{

			// sort by the set order - layer, shader, material and depth
			if (renderConfig.general.sortRenderQueue)
			{
				renderQueue->sortAll();
			}

			// now draw all renderables to the pass - start by begining the renderpass 
			cmdBuffer->createPrimary();
			
			vk::RenderPassBeginInfo beginInfo;
			if (clearAttachment)
			{
				beginInfo = renderpass.getBeginInfo(vk::ClearColorValue(renderConfig.general.backgroundColour));
			}
			else
			{
				beginInfo = renderpass.getBeginInfo();
			}

			cmdBuffer->beginRenderpass(beginInfo, true);

			// now draw everything in the designated queue 
			renderQueue->threadedDispatch(cmdBuffer, type);

			// end the primary pass and buffer
			cmdBuffer->endRenderpass();	
		}
	}

	PresentationPass::PresentationPass(RenderConfig& renderConfig)
	{
		// create the ubo buffer data now for the shader as this will remain static
		VulkanAPI::BufferUpdateEvent event{ "Present", (void*)&renderConfig.toneMapSettings, sizeof(RenderConfig::ToneMapSettings), VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };

		// let the buffer manager know that the buffers needs creating/updating via the event process
		Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
	}

	PresentationPass::~PresentationPass()
	{
	}

	void PresentationPass::createPipeline(vk::Device& device, vk::ImageView& postProcessImageView, VulkanAPI::Swapchain& swapchain, std::unique_ptr<VulkanAPI::BufferManager>& bufferManager)
	{
		
		if (!state.shader.add(device, "quad-vert.spv", VulkanAPI::StageType::Vertex, "PostProcess/final-composition-frag.spv", VulkanAPI::StageType::Fragment)) 
		{
			LOGGER_ERROR("Unable to create shaders for final composition.");
		}
			
		// get pipeline layout and vertedx attributes by reflection of shader
		state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
		state.shader.bufferReflection(state.descriptorLayout, state.bufferLayout);
		state.descriptorLayout.create(device);
		state.descriptorSet.init(device, state.descriptorLayout);

		// sort out the descriptor sets - buffers
		for (auto& layout : state.bufferLayout.layouts)
		{
			if (layout.name == "UboBuffer")
			{
				bufferManager->enqueueDescrUpdate("Present", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		for (auto& layout : state.imageLayout.layouts)
		{
			if (layout.name == "imageSampler")
			{
				auto& image = state.imageLayout.find(layout.set, layout.binding);
				if (image)
				{
					state.descriptorSet.writeSet(image.value(), postProcessImageView);
				}
				else
				{
					LOGGER_ERROR("Unable to find image sampler %s after shader reflection\n", layout.name.c_str());
				}
			}
		}

		state.shader.pipelineLayoutReflect(state.pipelineLayout);
		state.pipelineLayout.create(device, state.descriptorLayout.getLayout());

		// create the graphics pipeline
		state.shader.pipelineReflection(state.pipeline);

		state.pipeline.setDepthState(VK_TRUE, VK_FALSE);
		state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
		state.pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state.pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.addColourAttachment(VK_FALSE, swapchain.getRenderpass());
		state.pipeline.create(device, swapchain.getRenderpass(), state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void PresentationPass::render(std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager, RenderConfig& renderConfig, VulkanAPI::Swapchain& swapchain)
	{
		uint32_t imageCount = cmdBufferManager->getPresentImageCount();
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			auto& cmdBuffer = cmdBufferManager->beginPresentCmdBuffer(swapchain.getRenderpass(), renderConfig.general.backgroundColour, i);

			cmdBuffer->setViewport();
			cmdBuffer->setScissor();

			// bind everything required to draw
			cmdBuffer->bindPipeline(state.pipeline);
			cmdBuffer->bindDescriptors(state.pipelineLayout, state.descriptorSet, VulkanAPI::PipelineType::Graphics);

			// render full screen quad to screen
			cmdBuffer->drawQuad();

			// end this pass and cmd buffer
			cmdBuffer->endRenderpass();
			cmdBuffer->end();
		}
	}
	
}