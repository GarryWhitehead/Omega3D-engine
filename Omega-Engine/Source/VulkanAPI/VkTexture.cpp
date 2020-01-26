#include "VulkanAPI/VkTexture.h"

#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

Texture::~Texture()
{
}

void Texture::create2dTex(
    VkDriver& driver,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    uint8_t mipLevels,
    vk::ImageUsageFlags usageFlags)
{
    texContext = TextureContext {format, width, height, mipLevels, 1, 1};

    // create an empty image
    image = new Image(driver.getContext(), *this);
    image->create(driver.getVma(), usageFlags);

    // and a image view of the empty image
    imageView = new ImageView(driver.getContext());
    imageView->create(driver.getContext().getDevice(), *image);
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
    size_t size = texContext.width * texContext.height * 4 * texContext.mipLevels;

    StagingPool::StageInfo stage = stagePool.create(size);
    memcpy(stage.allocInfo.pMappedData, data, size);

    // create the info required for the copy
    std::vector<vk::BufferImageCopy> copyBuffers;
    if (texContext.faceCount == 1 && texContext.arrays == 1)
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
    std::unique_ptr<CmdBuffer> cmdBuffer = manager.getSingleUseCb();

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
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmdBuffer->get());

    cmdBuffer->flush();

    // clean up the staging area
    stagePool.release(stage);
    manager.releaseSingleUseCb(std::move(cmdBuffer));
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
    return imageView;
}

Image* Texture::getImage()
{
    return image;
}

Sampler* Texture::getSampler()
{
    return sampler;
}

vk::ImageLayout& Texture::getImageLayout()
{
    return imageLayout;
}

} // namespace VulkanAPI
