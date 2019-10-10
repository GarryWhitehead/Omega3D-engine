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
    gbufferInfo.position = builder.createTexture(
        "position", { .width = 2048, .height = 2048, .format = vk::Format::eR16G16B16A16Sfloat });
    gbufferInfo.colour = builder.createTexture("colour", { .width = 2048, .height = 2048, .format = vk::Format::eR8G8B8A8Unorm });
    gbufferInfo.normal = builder.createTexture("normal", { .width = 2048, .height = 2048, .format = vk::Format::eR8G8B8A8Unorm });
    gbufferInfo.pbr = builder.createTexture("pbr", { .width = 2048, .height = 2048, .format = vk::Format::eR16G16Sfloat });
    gbufferInfo.emissive = builder.createTexture(
        "emissive", { .width = 2048, .height = 2048, .format = vk::Format::eR16G16B16A16Sfloat });
    gbufferInfo.emissive = builder.createTexture("depth", { .width = 2048, .height = 2048, .format = depthFormat });

    // create the output taragets
    gbufferInfo.position = builder.addOutput(gbufferInfo.position);
    gbufferInfo.colour = builder.addOutput(gbufferInfo.colour);
    gbufferInfo.normal = builder.addOutput(gbufferInfo.normal);
    gbufferInfo.emissive = builder.addOutput(gbufferInfo.emissive);
    gbufferInfo.pbr = builder.addOutput(gbufferInfo.pbr);
    gbufferInfo.depth = builder.addOutput(gbufferInfo.depth);

        

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


}
