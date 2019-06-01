#include "IblInterface.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Shader.h"
#include "Vulkan/Queue.h"
#include "Vulkan/Image.h"
#include "Vulkan/Sampler.h"
#include "Rendering/StockModels.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

	IblInterface::IblInterface(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graphicsQueue)
	{
		generateBrdf(device, gpu, graphicsQueue);
	}


	IblInterface::~IblInterface()
	{
	}

	void IblInterface::generateBrdf(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graphicsQueue)
	{
		const uint32_t lutDimensions = 512;
		const vk::Format lutFormat = vk::Format::eR16G16Sfloat;
		vk::ClearColorValue clearValue;

		brdfTexture.init(device, gpu, graphicsQueue);
		brdfTexture.createEmptyImage(lutFormat, lutDimensions, lutDimensions, 1, vk::ImageUsageFlagBits::eColorAttachment);

		// setup renderpass
		VulkanAPI::RenderPass renderpass(device);
		renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, lutFormat);
		renderpass.prepareRenderPass();

		// and the frame buffer
		renderpass.prepareFramebuffer(brdfTexture.getImageView(), lutDimensions, lutDimensions);

		// prepare the shader
		VulkanAPI::Shader shader;
		shader.add(device, "quad-vert.spv", VulkanAPI::StageType::Vertex, "Env/lutBRDF-frag.spv", VulkanAPI::StageType::Fragment);

		// and the pipeline.....
		VulkanAPI::Pipeline pipeline;

		// create the graphics pipeline
		shader.pipelineReflection(pipeline);

		// no pipeline layout required as we have no buffers or samplers - so create empty
		pipeline.setDepthState(VK_FALSE, VK_FALSE);
		pipeline.setRasterCullMode(vk::CullModeFlagBits::eNone);
		pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
		pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		pipeline.addColourAttachment(VK_FALSE, renderpass);
		pipeline.create(device, renderpass, shader, VulkanAPI::PipelineType::Graphics);

		// and finally the command buffers
		VulkanAPI::CommandBuffer cmdBuffer(device, graphicsQueue.getIndex());
		cmdBuffer.createPrimary();

		vk::RenderPassBeginInfo beginInfo = renderpass.getBeginInfo(clearValue);
		cmdBuffer.beginRenderpass(beginInfo);

		cmdBuffer.setViewport();
		cmdBuffer.setScissor();
		cmdBuffer.bindPipeline(pipeline);
		cmdBuffer.drawQuad();

		cmdBuffer.endRenderpass();
		cmdBuffer.end();

		// push straight to the graphics queue
		graphicsQueue.flushCmdBuffer(cmdBuffer.get());
	}

	void IblInterface::generateIrradianceMap(vk::Device device, vk::PhysicalDevice& gpu, VulkanAPI::Queue& graphicsQueue)
	{
		const uint32_t irradiance_dim = 64;
		const uint8_t mipLevels = 5;
		vk::ClearColorValue clear_value;

		// cube texture
		VulkanAPI::Texture cube_tex(device, gpu, graphicsQueue);
		cube_tex.createEmptyImage(vk::Format::eR32G32B32A32Sfloat, irradiance_dim, irradiance_dim, mipLevels, vk::ImageUsageFlagBits::eColorAttachment);

		// offscreen texture
		VulkanAPI::Texture offscreen_tex(device, gpu, graphicsQueue);
		offscreen_tex.createEmptyImage(vk::Format::eR32G32B32A32Sfloat, irradiance_dim, irradiance_dim, mipLevels, vk::ImageUsageFlagBits::eColorAttachment);

		// renderpass and framebuffer
		VulkanAPI::RenderPass renderpass(device);
		renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR32G32B32A32Sfloat);
		renderpass.prepareRenderPass();
		renderpass.prepareFramebuffer(offscreen_tex.getImageView(), irradiance_dim, irradiance_dim);

		// prepare the shader
		VulkanAPI::DescriptorLayout descriptorLayout;
		VulkanAPI::Shader shader;
		VulkanAPI::PipelineLayout pipelineLayout;
		shader.add(device, "env/irradiance_map-vert.spv", VulkanAPI::StageType::Vertex, "env/irradiance-frag.spv", VulkanAPI::StageType::Fragment);

		// use reflection to fill in the pipeline layout and descriptors
		shader.pipelineLayoutReflect(pipelineLayout);
		VulkanAPI::ImageReflection sampler_layout;
		shader.imageReflection(descriptorLayout, sampler_layout);

		// descriptor sets
		VulkanAPI::DescriptorSet descriptorSet(device, descriptorLayout);
		VulkanAPI::Sampler linear_sampler(device, VulkanAPI::SamplerType::LinearClamp);
		//descriptorSet.writeSet(sampler_layout[0][0].set, sampler_layout[0][0].binding, vk::DescriptorType::eSampler, linear_sampler.getSampler(), cube_tex.getImageView(), vk::ImageLayout::eShaderReadOnlyOptimal);

		// pipeline
		VulkanAPI::Pipeline pipeline;
		pipeline.addShader(shader);
		pipeline.setRenderpass(renderpass);
		pipeline.addColourAttachment(VK_FALSE, renderpass);
		pipeline.addLayout(pipelineLayout.get());
		pipeline.create(device, VulkanAPI::PipelineType::Graphics);

		// use the stock cube mesh
		RenderUtil::CubeModel cube_model;

		// record command buffer
		VulkanAPI::CommandBuffer cmdBuffer(device, graphicsQueue.getIndex());
		vk::RenderPassBeginInfo beginInfo = renderpass.getBeginInfo(clear_value);

		// transition cube texture for transfer
		cube_tex.getImage().transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer.get());

		// record command buffer for each mip and their layers
		for (uint8_t mip = 0; mip < mipLevels; ++mip) {
			for (uint8_t layer = 0; layer < 6; ++layer) {

				// get dimensions for this mip level
				float mip_dim = static_cast<float>(irradiance_dim * std::pow(0.5, mip));
				vk::Viewport viewPort(mip_dim, mip_dim, 0.0f, 1.0f);

				cmdBuffer.beginRenderpass(beginInfo, viewPort);
				cmdBuffer.bindPipeline(pipeline);
				cmdBuffer.bindDescriptors(pipelineLayout, descriptorSet, VulkanAPI::PipelineType::Graphics);
				//cmdBuffer.bindVertexBuffer(cube_model.get_vertexBuffer());
				//cmdBuffer.bindIndexBuffer(cube_model.getIndexBuffer());

				// calculate view for each cube side
				FilterPushConstant push_block;
				push_block.mvp = OEMaths::perspective(static_cast<float>(M_PI) / 2.0f, 1.0f, 0.1f, 512.0f) * cubeView[layer];
				cmdBuffer.bindPushBlock(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, sizeof(FilterPushConstant), &push_block);

				// draw cube into offscreen framebuffer
				//cmdBuffer.drawIndexed(cube_model.getIndexCount());
				cmdBuffer.endRenderpass();

				// copy the offscreen buffer to the current layer
				vk::ImageSubresourceLayers src_resource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
				vk::Offset3D srcOffset(0, 0, 0);
				vk::ImageSubresourceLayers dst_resource(vk::ImageAspectFlagBits::eColor, mip, layer, 1);
				vk::Offset3D dstOffset(0, 0, 0);
				vk::Extent3D extent(static_cast<uint32_t>(mip_dim), static_cast<uint32_t>(mip_dim), 1);
				vk::ImageCopy imageCopy(src_resource, srcOffset, dst_resource, dstOffset, extent);

				offscreen_tex.getImage().transition(vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer.get());
				cmdBuffer.get().copyImage(offscreen_tex.getImage().get(), vk::ImageLayout::eTransferSrcOptimal, cube_tex.getImage().get(), vk::ImageLayout::eTransferDstOptimal, 1, &imageCopy);

				// transition the offscreen image back to colour attachment ready for the next image
				offscreen_tex.getImage().transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal, cmdBuffer.get());
			}
		}
		cube_tex.getImage().transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eUndefined, cmdBuffer.get());

		graphicsQueue.flushCmdBuffer(cmdBuffer.get());
	}

}
