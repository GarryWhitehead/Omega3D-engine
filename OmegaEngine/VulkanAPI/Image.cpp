#include "Image.h"
#include "Utility/logger.h"
#include "VulkanAPI/BufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include "VulkanAPI/Queue.h"

namespace VulkanAPI
{
ImageView::ImageView()
{
}

vk::ImageAspectFlags ImageView::getImageAspect(vk::Format format)
{
	vk::ImageAspectFlags aspect;

	switch (format)
	{
	// depth/stencil image formats
	case vk::Format::eD32SfloatS8Uint:
		[[__fallthrough]] case vk::Format::eD24UnormS8Uint
		    : aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		break;
		// depth only formats
	case vk::Format::eD32Sfloat:
		[[__fallthrough]] case vk::Format::eD16Unorm : aspect = vk::ImageAspectFlagBits::eDepth;
		break;
		// otherwist must be a colour format
	default:
		aspect = vk::ImageAspectFlagBits::eColor;
	}
	return aspect;
}

vk::ImageViewType ImageView::getTextureType(uint32_t faceCount, uint32_t arrayCount)
{
	if (arrayCount > 1 && faceCount == 1)
	{
		return vk::ImageViewType::e2DArray;    //only dealing with 2d textures at the moment
	}
	else if (faceCount == 6)
	{
		if (arrayCount == 1)
		{
			return vk::ImageViewType::eCube;
		}
		else
		{
			return vk::ImageViewType::eCubeArray;
		}
	}

	return vk::ImageViewType::e2D;
}

void ImageView::create(vk::Device dev, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspect,
                       vk::ImageViewType type)
{
	device = dev;

	vk::ImageViewCreateInfo createInfo({}, image, type, format,
	                                   { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
	                                     vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
	                                   { aspect, 0, 1, 0, 1 });

	VK_CHECK_RESULT(device.createImageView(&createInfo, nullptr, &imageView));
}

void ImageView::create(vk::Device dev, Image& image)
{
	device = dev;

	vk::ImageViewType type = getTextureType(image.getFaceCount(), image.getArrayCount());

	// making assumptions here based on the image format used
	vk::ImageAspectFlags aspect = getImageAspect(image.getFormat());

	vk::ImageViewCreateInfo createInfo({}, image.get(), type, image.getFormat(),
	                                   { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
	                                     vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
	                                   vk::ImageSubresourceRange(aspect, 0, 1, 0, 1));

	VK_CHECK_RESULT(device.createImageView(&createInfo, nullptr, &imageView));
}

Image::Image()
{
}

Image::~Image()
{
}

vk::Filter Image::getFilterType(vk::Format format)
{
	vk::Filter filter;

	if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint ||
	    format == vk::Format::eD32Sfloat || format == vk::Format::eD16Unorm)
	{
		filter = vk::Filter::eNearest;
	}
	else
	{
		filter = vk::Filter::eLinear;
	}
	return filter;
}

void Image::create(vk::Device dev, vk::PhysicalDevice& gpu, Texture& texture, vk::ImageUsageFlags usageFlags)
{
	device = dev;

	format = texture.getFormat();
	faceCount = texture.getFaceCount();
	arrays = texture.getArrayCount();
	mipLevels = texture.getMipLevels();
	width = texture.getWidth();
	height = texture.getHeight();

	vk::ImageCreateInfo image_info(
	    {}, vk::ImageType::e2D, format, { width, height, 1 }, mipLevels,
	    faceCount * arrays,    // remeber that faceCount are also considered to be array layers
	    vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | usageFlags,
	    vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);

	if (faceCount == 6)
	{
		image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;
	}

	VK_CHECK_RESULT(device.createImage(&image_info, nullptr, &image));

	// allocate memory for this image
	vk::MemoryRequirements requiredMemory = device.getImageMemoryRequirements(image);

	uint32_t memoryType =
	    Util::findMemoryType(requiredMemory.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, gpu);
	if (memoryType == UINT32_MAX)
	{
		LOGGER_ERROR("Unable to find required gpu memory type.");
	}
	vk::MemoryAllocateInfo allocateInfo(requiredMemory.size, memoryType);

	VK_CHECK_RESULT(device.allocateMemory(&allocateInfo, nullptr, &imageMemory));
	device.bindImageMemory(image, imageMemory, 0);
}

void Image::transition(vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer& cmdBuff,
                       uint32_t baseMipMapLevel)
{

	vk::ImageAspectFlags mask = ImageView::getImageAspect(format);

	vk::AccessFlags srcBarrier, dstBarrier;

	switch (oldLayout)
	{
	case vk::ImageLayout::eUndefined:
		srcBarrier = (vk::AccessFlagBits)0;
		break;
	case vk::ImageLayout::eTransferDstOptimal:
		srcBarrier = vk::AccessFlagBits::eTransferWrite;
		break;
	case vk::ImageLayout::eTransferSrcOptimal:
		srcBarrier = vk::AccessFlagBits::eTransferRead;
		break;
	case vk::ImageLayout::eColorAttachmentOptimal:
		srcBarrier = vk::AccessFlagBits::eColorAttachmentWrite;
		break;
	default:
		srcBarrier = (vk::AccessFlagBits)0;
	}

	switch (newLayout)
	{
	case vk::ImageLayout::eTransferDstOptimal:
		dstBarrier = vk::AccessFlagBits::eTransferWrite;
		break;
	case vk::ImageLayout::eTransferSrcOptimal:
		dstBarrier = vk::AccessFlagBits::eTransferRead;
		break;
	case vk::ImageLayout::eColorAttachmentOptimal:
		dstBarrier = vk::AccessFlagBits::eColorAttachmentWrite;
		break;
	case vk::ImageLayout::eShaderReadOnlyOptimal:
		dstBarrier = vk::AccessFlagBits::eShaderRead;
		break;
	case vk::ImageLayout::eDepthStencilAttachmentOptimal:
		dstBarrier = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		break;
	default:
		dstBarrier = (vk::AccessFlagBits)0;
	}

	vk::ImageSubresourceRange subresourceRange(mask, 0, mipLevels, 0, arrays * faceCount);

	if (baseMipMapLevel != UINT32_MAX)
	{
		subresourceRange.baseMipLevel = baseMipMapLevel;
		subresourceRange.levelCount = 1;
	}

	vk::ImageMemoryBarrier memoryBarrier(srcBarrier, dstBarrier, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED,
	                                     VK_QUEUE_FAMILY_IGNORED, image, subresourceRange);

	cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
	                        (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

// image-based functions =======
void Image::generateMipMap(vk::CommandBuffer cmdBuffer)
{
	for (uint8_t i = 1; i < mipLevels; ++i)
	{
		// source
		vk::ImageSubresourceLayers src(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
		vk::Offset3D srcOffset(width >> (i - 1), height >> (i - 1), 1);

		// destination
		vk::ImageSubresourceLayers dst(vk::ImageAspectFlagBits::eColor, i, 0, 1);
		vk::Offset3D dstOffset(width >> i, height >> i, 1);

		vk::ImageBlit imageBlit;
		imageBlit.srcSubresource = src;
		imageBlit.srcOffsets[1] = srcOffset;
		imageBlit.dstSubresource = dst;
		imageBlit.dstOffsets[1] = dstOffset;

		// create image barrier - transition image to transfer
		transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer, i);

		// blit the image
		cmdBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1,
		                    &imageBlit, vk::Filter::eLinear);
		transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer, i);
	}

	// prepare for shader read
	transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, cmdBuffer);
}

void Image::blit(VulkanAPI::Image& otherImage, VulkanAPI::Queue& graphicsQueue)
{
	// source
	vk::ImageAspectFlags imageAspect = ImageView::getImageAspect(format);

	vk::ImageSubresourceLayers src(imageAspect, 0, 0, 1);
	vk::Offset3D srcOffset(width, height, 1);

	// destination
	vk::ImageSubresourceLayers dst(imageAspect, 0, 0, 1);
	vk::Offset3D dstOffset(width, height, 1);

	vk::ImageBlit imageBlit;
	imageBlit.srcSubresource = src;
	imageBlit.srcOffsets[1] = srcOffset;
	imageBlit.dstSubresource = dst;
	imageBlit.dstOffsets[1] = dstOffset;

	// cmd buffer required for the image blit
	CommandBuffer blitCmdBuffer(device, graphicsQueue.getIndex());
	blitCmdBuffer.createPrimary();

	transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, blitCmdBuffer.get());
	otherImage.transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, blitCmdBuffer.get());

	// blit the image
	vk::Filter filter = getFilterType(format);
	blitCmdBuffer.get().blitImage(otherImage.get(), vk::ImageLayout::eTransferSrcOptimal, image,
	                              vk::ImageLayout::eTransferDstOptimal, 1, &imageBlit, filter);

	transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, blitCmdBuffer.get());
	otherImage.transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	                      blitCmdBuffer.get());

	// flush the cmd buffer
	blitCmdBuffer.end();
	graphicsQueue.flushCmdBuffer(blitCmdBuffer.get());
}
}    // namespace VulkanAPI
