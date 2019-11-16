#include "VulkanAPI/VkTexture.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Buffer.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

Texture::Texture() noexcept :
    image(std::make_unique<Image>()),
    imageView(std::make_unique<ImageView>())
{
}

Texture::~Texture()
{
}

void Texture::create2dTex(vk::Format format, uint32_t width, uint32_t height, uint8_t mipLevels, vk::ImageUsageFlags usageFlags)
{
    texContext = TextureContext{format, width, height, mipLevels, 1, 1};

	// create an empty image
	image.create(vkContext, *this, usageFlags);

	// and a image view of the empty image
	imageView.create(vkContext, image);
}

void Texture::map(StagingPool& stagePool, void* data)
{
	// using VMA here, hence no c++ bindings and left in a verbose format
	vk::DeviceMemory stagingMemory;
	vk::Buffer stagingBuffer;
    
    size_t size = texContext.width * texContext.height * texContext.mipLevels;
    
    StagingPool::StageInfo stage = stagePool.create(size);
    memcpy(stage.mem->GetMappedData(), data, size);

	// create the info required for the copy
	std::vector<vk::BufferImageCopy> copyBuffers;
	if (faceCount == 1 && arrays == 1)
	{
		createCopyBuffer(copyBuffers);
	}
	else
	{
		createArrayCopyBuffer(copyBuffers);
	}

	// now copy image to local device - first prepare the image for copying via transitioning to a transfer state. After copying, the image is transistioned ready for reading by the shader
	CommandBuffer copyCmdBuffer(device, graphicsQueue.getIndex());
	copyCmdBuffer.createPrimary();

	image.transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, copyCmdBuffer.get());
	copyCmdBuffer.get().copyBufferToImage(stagingBuffer, image.get(), vk::ImageLayout::eTransferDstOptimal,
	                                      static_cast<uint32_t>(copyBuffers.size()), copyBuffers.data());
	image.transition(vk::ImageLayout::eTransferDstOptimal, finalTransitionLayout, copyCmdBuffer.get());

	copyCmdBuffer.end();
	graphicsQueue.flushCmdBuffer(copyCmdBuffer.get());
    
    // clean up the staging area
    stagePool.release(stage);
}

void Texture::createCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers)
{
	uint32_t offset = 0;
	for (uint32_t level = 0; level < mipLevels; ++level)
	{
		vk::BufferImageCopy imageCopy(offset, 0, 0,
		                              vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, level, 0, 1),
		                              vk::Offset3D(0, 0, 0), vk::Extent3D(width >> level, height >> level, 1));
		copyBuffers.emplace_back(imageCopy);

		offset += (width >> level) * (height >> level);
	}
}

void Texture::createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers)
{
	uint32_t offset = 0;

	for (uint32_t face = 0; face < faceCount; ++face)
	{
		for (uint32_t level = 0; level < mipLevels; ++level)
		{

			vk::BufferImageCopy imageCopy(offset, 0, 0, { vk::ImageAspectFlagBits::eColor, level, face, 1 },
			                              { 0, 0, 0 }, { width >> level, height >> level, 1 });
			copyBuffers.emplace_back(imageCopy);

			offset += (width >> level) * (height >> level) * 4;
		}
	}
}

vk::ImageView& Texture::getImageView()
{
	return imageView.getImageView();
}

}    // namespace VulkanAPI
