#include "Image.h"

#include "utility/Logger.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

// ================ ImageView =============================

ImageView::ImageView(VkContext& context) :
    device(context.getDevice())
{
}

ImageView::~ImageView()
{
    device.destroy(imageView, nullptr);
}

vk::ImageAspectFlags ImageView::getImageAspect(vk::Format format)
{
	vk::ImageAspectFlags aspect;

	switch (format)
	{
	// depth/stencil image formats
	case vk::Format::eD32SfloatS8Uint:
    case vk::Format::eD24UnormS8Uint:
        aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		break;
		// depth only formats
	case vk::Format::eD32Sfloat:
    case vk::Format::eD16Unorm:
        aspect = vk::ImageAspectFlagBits::eDepth;
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
    
	vk::ImageViewType type = getTextureType(image.getContext().faceCount, image.getContext().arrays);

	// making assumptions here based on the image format used
	vk::ImageAspectFlags aspect = getImageAspect(image.getContext().format);

	vk::ImageViewCreateInfo createInfo({}, image.get(), type, image.getContext().format,
	                                   { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,
	                                     vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
	                                   vk::ImageSubresourceRange(aspect, 0, 1, 0, 1));

	VK_CHECK_RESULT(device.createImageView(&createInfo, nullptr, &imageView));
}

// ==================== Image ===================

Image::Image(VkContext& context, Texture& tex) :
    device(context.getDevice()),
    tex(tex.getContext())
{
}

Image::~Image()
{
    device.destroy(image, nullptr);
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

void Image::create(VmaAllocator& vmaAlloc, vk::ImageUsageFlags usageFlags)
{
    vk::ImageCreateInfo imageInfo = { {}, vk::ImageType::e2D, tex.format, {1, tex.width, tex.height}, tex.mipLevels, 0, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | usageFlags, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined };
    
    if (tex.faceCount == 6)
    {
        imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    }
    
    // so we can use the vulkan c++ style, and VMA uses the C style, memcpy into the C style struct
    VkImageCreateInfo vkImageInfo = {};
    memcpy(&vkImageInfo, &imageInfo, sizeof(VkImageCreateInfo));
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VkImage tempImage;
    VMA_CHECK_RESULT(vmaCreateImage(vmaAlloc, &vkImageInfo, &allocInfo, &tempImage, &imageMem, nullptr));
    image = tempImage;
}

void Image::transition(Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer& cmdBuff,
                       uint32_t baseMipMapLevel)
{
    TextureContext& tex = image.getContext();
    
	vk::ImageAspectFlags mask = ImageView::getImageAspect(tex.format);

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

	vk::ImageSubresourceRange subresourceRange(mask, 0, tex.mipLevels, 0, tex.arrays * tex.faceCount);

	if (baseMipMapLevel != UINT32_MAX)
	{
		subresourceRange.baseMipLevel = baseMipMapLevel;
		subresourceRange.levelCount = 1;
	}

	vk::ImageMemoryBarrier memoryBarrier(srcBarrier, dstBarrier, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, image.get(), subresourceRange);

	cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
	                        (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
}

// image-based functions =======
void Image::generateMipMap(Image& image, CmdBuffer& cmdBuffer)
{
    TextureContext& tex = image.getContext();
    
    for (uint8_t i = 1; i < tex.mipLevels; ++i)
	{
		// source
		vk::ImageSubresourceLayers src(vk::ImageAspectFlagBits::eColor, i - 1, 0, 1);
		vk::Offset3D srcOffset(tex.width >> (i - 1), tex.height >> (i - 1), 1);

		// destination
		vk::ImageSubresourceLayers dst(vk::ImageAspectFlagBits::eColor, i, 0, 1);
		vk::Offset3D dstOffset(tex.width >> i, tex.height >> i, 1);

		vk::ImageBlit imageBlit;
		imageBlit.srcSubresource = src;
		imageBlit.srcOffsets[1] = srcOffset;
		imageBlit.dstSubresource = dst;
		imageBlit.dstOffsets[1] = dstOffset;

		// create image barrier - transition image to transfer
		transition(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer.get(), i);

		// blit the image
		cmdBuffer.get().blitImage(image.get(), vk::ImageLayout::eTransferSrcOptimal, image.get(), vk::ImageLayout::eTransferDstOptimal, 1,
		                    &imageBlit, vk::Filter::eLinear);
        
		transition(image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer.get(), i);
	}

	// prepare for shader read
	transition(image, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, cmdBuffer.get());
}

void Image::blit(Image& srcImage, Image& dstImage, CmdBuffer& cmdBuffer)
{
	// source
    TextureContext& tex = srcImage.getContext();
	vk::ImageAspectFlags imageAspect = ImageView::getImageAspect(tex.format);

	vk::ImageSubresourceLayers src(imageAspect, 0, 0, 1);
	vk::Offset3D srcOffset(tex.width, tex.height, 1);

	// destination
	vk::ImageSubresourceLayers dst(imageAspect, 0, 0, 1);
	vk::Offset3D dstOffset(tex.width, tex.height, 1);

	vk::ImageBlit imageBlit;
	imageBlit.srcSubresource = src;
	imageBlit.srcOffsets[1] = srcOffset;
	imageBlit.dstSubresource = dst;
	imageBlit.dstOffsets[1] = dstOffset;

	transition(srcImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer.get());
	transition(dstImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer.get());

	// blit the image
	vk::Filter filter = getFilterType(tex.format);
	cmdBuffer.get().blitImage(dstImage.get(), vk::ImageLayout::eTransferSrcOptimal, srcImage.get(), vk::ImageLayout::eTransferDstOptimal, 1, &imageBlit, filter);

	transition(srcImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, cmdBuffer.get());
	transition(dstImage, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	                      cmdBuffer.get());

	// flush the cmd buffer
	cmdBuffer.flush();
}
}    // namespace VulkanAPI
