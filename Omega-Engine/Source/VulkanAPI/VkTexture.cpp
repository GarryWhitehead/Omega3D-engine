#include "VulkanAPI/VkTexture.h"

#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

Texture::~Texture()
{
}

enum Filter
{
    Nearest,
    Linear,
    Cubic
};

enum AddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

vk::Filter Texture::toVkFilter(const OEFilter filter)
{
    vk::Filter result;

    switch (filter)
    {
        case OEFilter::Nearest:
            result = vk::Filter::eNearest;
            break;
        case OEFilter::Linear:
            result = vk::Filter::eLinear;
            break;
        case OEFilter::Cubic:
            result = vk::Filter::eCubicIMG;
            break;
    }
    return result;
}

vk::SamplerAddressMode Texture::toVkAddressMode(const OEAddressMode mode)
{
    vk::SamplerAddressMode result;

    switch (mode)
    {
        case OEAddressMode::Repeat:
            result = vk::SamplerAddressMode::eRepeat;
            break;
        case OEAddressMode::MirroredRepeat:
            result = vk::SamplerAddressMode::eMirroredRepeat;
            break;
        case OEAddressMode::ClampToEdge:
            result = vk::SamplerAddressMode::eClampToEdge;
            break;
        case OEAddressMode::ClampToBorder:
            result = vk::SamplerAddressMode::eClampToBorder;
            break;
        case OEAddressMode::MirrorClampToEdge:
            result = vk::SamplerAddressMode::eMirrorClampToEdge;
            break;
    }
    return result;
}

void Texture::create2dTex(
    VkDriver& driver,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    uint8_t mipLevels,
    uint8_t faceCount,
    uint8_t arrayCount,
    vk::ImageUsageFlags usageFlags)
{
    texContext = TextureContext {format, width, height, mipLevels, faceCount, arrayCount};

    // create an empty image
    image = new Image(driver.getContext(), *this);
    image->create(driver.getVma(), usageFlags);

    // and a image view of the empty image
    imageView = new ImageView(driver.getContext());
    imageView->create(driver.getContext().device, *image);

    // create a defaut sampler
    createSampler(
        driver.getContext(),
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerAddressMode::eClampToEdge,
        vk::SamplerAddressMode::eClampToEdge,
        8.0f);

    if (VkUtil::isDepth(format) || VkUtil::isStencil(format))
    {
        imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    }
    else
    {
        imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    }
}

void Texture::createSampler(
    const VkContext& context,
    vk::Filter magFilter,
    vk::Filter minFilter,
    vk::SamplerAddressMode addrModeU,
    vk::SamplerAddressMode addrModeV,
    float maxAntriopsy)
{
    // override the default sampler
    if (sampler)
    {
        context.device.destroySampler(sampler);
    }

    vk::SamplerCreateInfo samplerInfo(
        {},
        magFilter,
        minFilter,
        vk::SamplerMipmapMode::eNearest,
        addrModeU,
        addrModeV,
        addrModeV,
        0.0f,
        VK_TRUE,
        maxAntriopsy, // TODO: add user control of max antriospy
        VK_FALSE,
        vk::CompareOp::eNever,
        0.0f,
        texContext.mipLevels, // maxLod should equal the mip-map count?
        vk::BorderColor::eIntOpaqueWhite,
        VK_FALSE);

    VK_CHECK_RESULT(context.device.createSampler(&samplerInfo, nullptr, &sampler));
}

void Texture::createSampler(
    const VkContext& context, const vk::SamplerCreateInfo& samplerCreateInfo)
{
    VK_CHECK_RESULT(context.device.createSampler(&samplerCreateInfo, nullptr, &sampler));
}

void Texture::destroy()
{
    if (imageView)
    {
        delete imageView;
        imageView = nullptr;
    }
    if (image)
    {
        delete image;
        image = nullptr;
    }
}

void Texture::map(VkDriver& driver, StagingPool& stagePool, void* data)
{
    // all images must be rgba
    size_t size = texContext.width * texContext.height * 4 * texContext.mipLevels * texContext.faceCount;

    StagingPool::StageInfo stage = stagePool.create(size);
    memcpy(stage.allocInfo.pMappedData, data, size);

    // create the info required for the copy
    std::vector<vk::BufferImageCopy> copyBuffers;
    if (texContext.faceCount == 1 && texContext.arrayCount == 1)
    {
        createCopyBuffer(copyBuffers);
    }
    else
    {
        createArrayCopyBuffer(copyBuffers);
    }

    // now copy image to local device - first prepare the image for copying via transitioning to a
    // transfer state. After copying, the image is transistioned ready for reading by the shader
    auto& manager = driver.getCbManager();
    auto* cmdBuffer = manager.getWorkCmdBuffer();

    Image::transition(
        *image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        cmdBuffer->get());

    cmdBuffer->get().copyBufferToImage(
        stage.buffer,
        image->get(),
        vk::ImageLayout::eTransferDstOptimal,
        static_cast<uint32_t>(copyBuffers.size()),
        copyBuffers.data());

    // the final transition here may need improving on....
    Image::transition(
        *image,
        vk::ImageLayout::eTransferDstOptimal,
        this->imageLayout,
        cmdBuffer->get());

    cmdBuffer->flush();

    // clean up the staging area
    stagePool.release(stage);
}

void Texture::createCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers)
{
    uint32_t offset = 0;
    for (uint32_t level = 0; level < texContext.mipLevels; ++level)
    {
        vk::BufferImageCopy imageCopy(
            offset,
            0,
            0,
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, level, 0, 1),
            vk::Offset3D(0, 0, 0),
            vk::Extent3D(texContext.width >> level, texContext.height >> level, 1));
        copyBuffers.emplace_back(imageCopy);

        offset += (texContext.width >> level) * (texContext.height >> level);
    }
}

void Texture::createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers)
{
    uint32_t offset = 0;

    for (uint32_t face = 0; face < texContext.faceCount; ++face)
    {
        for (uint32_t level = 0; level < texContext.mipLevels; ++level)
        {

            vk::BufferImageCopy imageCopy(
                offset,
                0,
                0,
                {vk::ImageAspectFlagBits::eColor, level, face, 1},
                {0, 0, 0},
                {texContext.width >> level, texContext.height >> level, 1});
            copyBuffers.emplace_back(imageCopy);

            offset += (texContext.width >> level) * (texContext.height >> level) * 4;
        }
    }
}

TextureContext& Texture::getContext()
{
    return texContext;
}

ImageView* Texture::getImageView()
{
    assert(imageView);
    return imageView;
}

Image* Texture::getImage()
{
    assert(image);
    return image;
}

vk::Sampler& Texture::getSampler()
{
    assert(sampler);
    return sampler;
}

vk::ImageLayout& Texture::getImageLayout()
{
    return imageLayout;
}

} // namespace VulkanAPI
