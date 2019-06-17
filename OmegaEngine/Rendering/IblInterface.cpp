#include "IblInterface.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Queue.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/Interface.h"
#include "VulkanAPI/VkTextureManager.h"
#include "VulkanAPI/BufferManager.h"
#include "Rendering/StockModels.h"
#include "Rendering/RenderCommon.h"
#include "OEMaths/OEMaths_transform.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	IblInterface::IblInterface(VulkanAPI::Interface& vkInterface)
	{
		// we can generate the brdf image straight away - the irradiance and pre-filtered maps
		// will be generated on the first render call as these are reliant on the skybox sampler
		generateBrdf(vkInterface);

		// we also need to init the textures, as the pipelines will require the texture image views upfront
		// cube texture
		irradianceMapTexture.init(vkInterface.getDevice(), vkInterface.getGpu(), vkInterface.getGraphicsQueue());
		irradianceMapTexture.createEmptyImage(vk::Format::eR32G32B32A32Sfloat, irradianceMapDim, irradianceMapDim, 
			mipLevels, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, 6);
	}


	IblInterface::~IblInterface()
	{
	}

	void IblInterface::generateBrdf(VulkanAPI::Interface& vkInterface)
	{
		const uint32_t lutDimensions = 512;
		const vk::Format lutFormat = vk::Format::eR16G16Sfloat;
		vk::ClearColorValue clearValue;

		brdfTexture.init(vkInterface.getDevice(), vkInterface.getGpu(), vkInterface.getGraphicsQueue());
		brdfTexture.createEmptyImage(lutFormat, lutDimensions, lutDimensions, 1, vk::ImageUsageFlagBits::eColorAttachment| vk::ImageUsageFlagBits::eSampled);

		// setup renderpass
		VulkanAPI::RenderPass renderpass(vkInterface.getDevice());
		renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, lutFormat);
		renderpass.prepareRenderPass();

		// and the frame buffer
		renderpass.prepareFramebuffer(brdfTexture.getImageView(), lutDimensions, lutDimensions);

		// prepare the shader
		VulkanAPI::Shader shader;
		if (!shader.add(vkInterface.getDevice(), "quad-vert.spv", VulkanAPI::StageType::Vertex, "Env/BRDF/lutBRDF-frag.spv", VulkanAPI::StageType::Fragment))
		{
			LOGGER_ERROR("Error. Unable to open brdf shader.\n");
		}
		
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
		pipeline.create(vkInterface.getDevice(), renderpass, shader, VulkanAPI::PipelineType::Graphics);

		// and finally the command buffers
		VulkanAPI::CommandBuffer cmdBuffer(vkInterface.getDevice(), vkInterface.getGraphicsQueue().getIndex());
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
		vkInterface.getGraphicsQueue().flushCmdBuffer(cmdBuffer.get());
	}

	void IblInterface::createIrradianceMap(VulkanAPI::Interface& vkInterface)
	{
		vk::ClearColorValue clearValue;
		ProgramState state;

		// offscreen texture - this will only be used for generating the map
		VulkanAPI::Texture offscreenTexture(vkInterface.getDevice(), vkInterface.getGpu(), vkInterface.getGraphicsQueue());
		offscreenTexture.createEmptyImage(vk::Format::eR32G32B32A32Sfloat, irradianceMapDim, irradianceMapDim, 1, 
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);

		// renderpass and framebuffer
		VulkanAPI::RenderPass renderPass(vkInterface.getDevice());
		renderPass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, vk::Format::eR32G32B32A32Sfloat);
		renderPass.prepareRenderPass();
		renderPass.prepareFramebuffer(offscreenTexture.getImageView(), irradianceMapDim, irradianceMapDim);

		// prepare the shader
		if (!state.shader.add(vkInterface.getDevice(), "env/irradiance/irradiance_map-vert.spv", VulkanAPI::StageType::Vertex, "env/irradiance/irradiance_map-frag.spv", VulkanAPI::StageType::Fragment))
		{
			LOGGER_ERROR("Error. Unable to open irradiance map shader.\n");
		}

		// use reflection to fill in the pipeline layout and descriptors
		state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
		state.shader.bufferReflection(state.descriptorLayout, state.bufferLayout);
		state.descriptorLayout.create(vkInterface.getDevice());
		state.descriptorSet.init(vkInterface.getDevice(), state.descriptorLayout);

		for (auto& layout : state.imageLayout.layouts)
		{
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "envSampler")
			{
				state.descriptorSet.writeSet(state.imageLayout.find(0, layout.binding).value(), vkInterface.gettextureManager()->getTextureImageView("Skybox"));
			}
		}

		// get the pipleine layout detail through shader reflection
		state.shader.pipelineLayoutReflect(state.pipelineLayout);
		state.pipelineLayout.create(vkInterface.getDevice(), state.descriptorLayout.getLayout());

		// pipeline
		state.shader.pipelineReflection(state.pipeline);

		state.pipeline.setDepthState(VK_FALSE, VK_FALSE);
		state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eNone);
		state.pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
		state.pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.addColourAttachment(VK_FALSE, renderPass);
		state.pipeline.create(vkInterface.getDevice(), renderPass, state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);

		// get the buffers for the cube model vertices/indices
		auto& vertexBuffer = vkInterface.getBufferManager()->getBuffer("CubeModelVertices");
		auto& indexBuffer = vkInterface.getBufferManager()->getBuffer("CubeModelIndices");

		// record command buffer
		VulkanAPI::CommandBuffer cmdBuffer(vkInterface.getDevice(), vkInterface.getGraphicsQueue().getIndex());
		cmdBuffer.createPrimary();
		vk::RenderPassBeginInfo beginInfo = renderPass.getBeginInfo(clearValue);

		// transition cube texture for transfer
		irradianceMapTexture.getImage().transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer.get());

		offscreenTexture.getImage().transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, cmdBuffer.get());

		// record command buffer for each mip and their layers
		for (uint8_t mip = 0; mip < mipLevels; ++mip) 
		{
			for (uint8_t face = 0; face < 6; ++face) 
			{
				// get dimensions for this mip level
				float mipDimensions = static_cast<float>(irradianceMapDim * std::pow(0.5, mip));
				vk::Viewport viewPort{ 0.0f, 0.0f, mipDimensions, mipDimensions, 0.0f, 1.0f };

				cmdBuffer.beginRenderpass(beginInfo, viewPort);
				cmdBuffer.setViewport();
				cmdBuffer.setScissor();

				cmdBuffer.bindPipeline(state.pipeline);
				cmdBuffer.bindDescriptors(state.pipelineLayout, state.descriptorSet, VulkanAPI::PipelineType::Graphics);
				cmdBuffer.bindVertexBuffer(vertexBuffer.buffer, vertexBuffer.offset);
				cmdBuffer.bindIndexBuffer(indexBuffer.buffer, indexBuffer.offset);

				// calculate view for each cube side
				IrradianceMapPushBlock pushBlock;
				OEMaths::mat4f mvp = OEMaths::perspective(static_cast<float>(M_PI) / 2.0f, 1.0f, 0.1f, 512.0f) * cubeView[face];
				cmdBuffer.bindPushBlock(state.pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(OEMaths::mat4f), &mvp);

				// draw cube into offscreen framebuffer
				cmdBuffer.drawIndexed(RenderUtil::CubeModel::indicesSize);
				cmdBuffer.endRenderpass();
				
				// copy the offscreen buffer to the current face
				vk::ImageSubresourceLayers src_resource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
				vk::ImageSubresourceLayers dst_resource(vk::ImageAspectFlagBits::eColor, mip, face, 1);
				vk::ImageCopy imageCopy(src_resource, 
										{ 0, 0, 0 }, 
										dst_resource, 
										{ 0, 0, 0 }, 
										{ static_cast<uint32_t>(mipDimensions), static_cast<uint32_t>(mipDimensions), 1 });

				offscreenTexture.getImage().transition(vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer.get());
				cmdBuffer.get().copyImage(offscreenTexture.getImage().get(), vk::ImageLayout::eTransferSrcOptimal, irradianceMapTexture.getImage().get(), vk::ImageLayout::eTransferDstOptimal, 1, &imageCopy);

				// transition the offscreen image back to colour attachment ready for the next image
				offscreenTexture.getImage().transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eColorAttachmentOptimal, cmdBuffer.get());
			}
		}
		irradianceMapTexture.getImage().transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, cmdBuffer.get());
		cmdBuffer.end();

		vkInterface.getGraphicsQueue().flushCmdBuffer(cmdBuffer.get());
	}

	void IblInterface::renderMaps(VulkanAPI::Interface& vkInterface)
	{
		createIrradianceMap(vkInterface); 

		mapsRendered = true;
	}
}