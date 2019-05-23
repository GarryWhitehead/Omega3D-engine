#include "RenderCommon.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Skybox.h"

namespace OmegaEngine
{

	namespace Rendering
	{
		void renderObjects(std::unique_ptr<RenderQueue>& renderQueue,
			VulkanAPI::RenderPass& renderpass,
			std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer,
			QueueType type,
			RenderConfig& renderConfig)
		{

			// sort by the set order - layer, shader, material and depth
			if (renderConfig.general.sortRenderQueue)
			{
				renderQueue->sortAll();
			}

			// now draw all renderables to the pass - start by begining the renderpass 
			cmdBuffer->createPrimary();
			vk::RenderPassBeginInfo beginInfo = renderpass.getBeginInfo(vk::ClearColorValue(renderConfig.general.backgroundColour));
			cmdBuffer->beginRenderpass(beginInfo, true);

			// now draw everything in the designated queue 
			renderQueue->threadedDispatch(cmdBuffer, type);

			// end the primary pass and buffer
			cmdBuffer->endRenderpass();
			cmdBuffer->end();
		}
	}

	PresentationPass::PresentationPass()
	{
	}

	PresentationPass::~PresentationPass()
	{
	}

	void PresentationPass::createPipeline()
	{
		
		if (!state.shader.add(device, "quad-vert.spv", VulkanAPI::StageType::Vertex, "presentation-frag.spv", VulkanAPI::StageType::Fragment)) 
		{
			LOGGER_ERROR("Unable to create static model shaders.");
		}
			
		// get pipeline layout and vertedx attributes by reflection of shader
		state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
		state.shader.bufferReflection(state.descriptorLayout, state.bufferLayout);
		state.descriptorLayout.create(device);
		state.descriptorSet.init(device, state.descriptorLayout);

		// sort out the descriptor sets - buffers
		for (auto& layout : state.bufferLayout) {

		}

		state.shader.pipelineLayoutReflect(state.pipelineLayout);
		state.pipelineLayout.create(device, state.descriptorLayout.getLayout());

		// create the graphics pipeline
		state.shader.pipelineReflection(state.pipeline);

		state.pipeline.setDepthState(VK_TRUE, VK_FALSE);
		state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eFront);
		state.pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state.pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.create(device, renderer->getShadowPass(), state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void PresentationPass::render(std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager, RenderConfig& renderConfig)
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
			cmdBuffer->bindPushBlock(state.pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(RenderConfig::IBLInfo), &renderConfig.ibl);

			// render full screen quad to screen
			cmdBuffer->drawQuad();

			// end this pass and cmd buffer
			cmdBuffer->endRenderpass();
			cmdBuffer->end();
		}
	}
	
}