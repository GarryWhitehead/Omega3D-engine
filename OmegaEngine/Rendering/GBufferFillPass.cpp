#include "GBufferFillPass.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/common.h"

namespace OmegaEngine
{

void GBufferFillPass::init()
{

	// a list of the formats required for each buffer
	vk::Format depthFormat = VulkanAPI::VkContext::getDepthFormat(gpu);

	RenderGraphBuilder builder = rGraph->createRenderPass("GBuffer Pass");

	// create the gbuffer textures
	gbufferInfo.tex.position = builder.createTexture(2048, 2048, vk::Format::eR16G16B16A16Sfloat);
	gbufferInfo.tex.colour = builder.createTexture(2048, 2048, vk::Format::eR8G8B8A8Unorm);
	gbufferInfo.tex.normal = builder.createTexture(2048, 2048, vk::Format::eR8G8B8A8Unorm);
	gbufferInfo.tex.pbr = builder.createTexture(2048, 2048, vk::Format::eR16G16Sfloat);
	gbufferInfo.tex.emissive = builder.createTexture(2048, 2048, vk::Format::eR16G16B16A16Sfloat);
	gbufferInfo.tex.depth = builder.createTexture(2048, 2048, depthFormat);

	// create the output taragets
	gbufferInfo.attach.position = builder.addOutputAttachment("position", gbufferInfo.tex.position);
	gbufferInfo.attach.colour = builder.addOutputAttachment("colour", gbufferInfo.tex.colour);
	gbufferInfo.attach.normal = builder.addOutputAttachment("normal", gbufferInfo.tex.normal);
	gbufferInfo.attach.emissive = builder.addOutputAttachment("emissive", gbufferInfo.tex.emissive);
	gbufferInfo.attach.pbr = builder.addOutputAttachment("pbr", gbufferInfo.tex.pbr);
	gbufferInfo.attach.depth = builder.addOutputAttachment("depth", gbufferInfo.tex.depth);


	// create a empty texture for each state - these will be filled by the shader
	for (uint8_t i = 0; i < attachmentCount; ++i)
	{
		if (i == (int)gBufferImageIndex::Depth)
		{
			gBufferImages[i].createEmptyImage(depthFormat, renderConfig.deferred.gBufferWidth,
			                                  renderConfig.deferred.gBufferHeight, 1,
			                                  vk::ImageUsageFlagBits::eDepthStencilAttachment);
		}
		else
		{
			gBufferImages[i].createEmptyImage(
			    firstRenderpass.get_attachment_format(i), renderConfig.deferred.gBufferWidth,
			    renderConfig.deferred.gBufferHeight, 1,
			    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
		}
	}

	// tie the image-views to the frame buffer
	std::vector<vk::ImageView> imageViews(attachmentCount);

	for (uint8_t i = 0; i < attachmentCount; ++i)
	{
		imageViews[i] = gBufferImages[i].getImageView();
	}

	firstRenderpass.prepareFramebuffer(static_cast<uint32_t>(imageViews.size()), imageViews.data(),
	                                   renderConfig.deferred.gBufferWidth, renderConfig.deferred.gBufferHeight);
}


}    // namespace OmegaEngine
