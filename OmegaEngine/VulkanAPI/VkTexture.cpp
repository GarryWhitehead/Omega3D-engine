#include "VulkanAPI/VkTexture.h"

#include "VulkanAPI/CommandBuffer.h"

#include "VulkanAPI/Image.h"

#include "VulkanAPI/RenderPass.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::init(VkContext& context) 
{
}

vk::Format Texture::convertTextureFormatToVulkan(OmegaEngine::TextureFormat format)
{
	vk::Format outputFormat;

	switch (format)
	{
	case OmegaEngine::TextureFormat::Image8UC4:
		outputFormat = vk::Format::eR8G8B8A8Unorm;
		break;
	case OmegaEngine::TextureFormat::Image16UC4:
		outputFormat = vk::Format::eR16G16B16A16Unorm;
		break;
	case OmegaEngine::TextureFormat::ImageBC3:
		outputFormat = vk::Format::eBc3UnormBlock;
		break;
	default:
		LOGGER_INFO("Unsupported texture format type - no conversion to vulkan type possible.");
	}

	return outputFormat;
}

void Texture::create(vk::Format format, uint32_t width, uint32_t height, uint8_t mipLevels,
                               vk::ImageUsageFlags usageFlags, uint32_t faces)
{
	assert(device);

	this->format = format;
	this->width = width;
	this->height = height;
	this->mipLevels = mipLevels;
	this->faceCount = faces;

	// create an empty image
	image.create(device, gpu, *this, usageFlags);

	// and a image view of the empty image
	imageView.create(device, image);
}

void Texture::map(OmegaEngine::MappedTexture& tex)
{
	assert(device);

	// store some of the texture attributes locally
	format = convertTextureFormatToVulkan(tex.getFormat());

	// using VMA here, hence no c++ bindings and left in a verbose format
	vk::DeviceMemory stagingMemory;
	vk::Buffer stagingBuffer;

	void* mappedData;
	device.mapMemory(stagingMemory, 0, tex.getSize(), {}, &mappedData);
	memcpy(mappedData, tex.data(), tex.getSize());
	device.unmapMemory(stagingMemory);

	// if generating mip maps, then we need to set the transfer and destination usage flags too
	vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eSampled;
	vk::ImageLayout finalTransitionLayout = RenderPass::getFinalTransitionLayout(format);

	if (mipLevels > 1)
	{
		usageFlags |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
	}

	image.create(device, gpu, *this, usageFlags);

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

	// noew copy image to local device - first prepare the image for copying via transitioning to a transfer state. After copying, the image is transistioned ready for reading by the shader
	CommandBuffer copyCmdBuffer(device, graphicsQueue.getIndex());
	copyCmdBuffer.createPrimary();

	image.transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, copyCmdBuffer.get());
	copyCmdBuffer.get().copyBufferToImage(stagingBuffer, image.get(), vk::ImageLayout::eTransferDstOptimal,
	                                      static_cast<uint32_t>(copyBuffers.size()), copyBuffers.data());
	image.transition(vk::ImageLayout::eTransferDstOptimal, finalTransitionLayout, copyCmdBuffer.get());

	copyCmdBuffer.end();
	graphicsQueue.flushCmdBuffer(copyCmdBuffer.get());

	// generate mip maps if required
	if (mipLevels > 1)
	{
		CommandBuffer blitCmdBuffer(device, graphicsQueue.getIndex());
		blitCmdBuffer.createPrimary();

		image.generateMipMap(blitCmdBuffer.get());

		blitCmdBuffer.end();
		graphicsQueue.flushCmdBuffer(blitCmdBuffer.get());
	}

	// create an image view of the texture image
	imageView.create(device, image);

	device.destroyBuffer(stagingBuffer, nullptr);
	device.freeMemory(stagingMemory, nullptr);
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
