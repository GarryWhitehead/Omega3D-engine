#include "Image.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Queue.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Utility/logger.h"

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
			[[__fallthrough]]
		case vk::Format::eD24UnormS8Uint:
			aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			break;
			// depth only formats
		case vk::Format::eD32Sfloat:
			[[__fallthrough]]
		case vk::Format::eD16Unorm:
			aspect = vk::ImageAspectFlagBits::eDepth;
			break;
			// otherwist must be a colour format
		default:
			aspect = vk::ImageAspectFlagBits::eColor;
		}
		return aspect;
	}

	vk::ImageViewType ImageView::getTexture_type(uint32_t faceCount, uint32_t arrayCount)
	{
		if (arrayCount > 1 && faceCount == 1)
		{
			return vk::ImageViewType::e2DArray;		//only dealing with 2d textures at the moment
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

	void ImageView::create(vk::Device dev, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type)
	{
		device = dev;

		vk::ImageViewCreateInfo createInfo({},
			image, type, format,
			{ vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity },
			{ aspect, 0, 1, 0, 1 });

		VK_CHECK_RESULT(device.createImageView(&createInfo, nullptr, &image_view));
	}

	void ImageView::create(vk::Device dev, Image& image)
	{
		device = dev;

		vk::ImageViewType type = getTexture_type(image.get_faceCount(), image.getArrayCount());

		// making assumptions here based on the image format used
		vk::ImageAspectFlags aspect = getImageAspect(image.getFormat());

		vk::ImageViewCreateInfo createInfo({},
			image.get(), type, image.getFormat(),
			{ vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity },
			vk::ImageSubresourceRange(aspect, 0, 1, 0, 1 ));

		VK_CHECK_RESULT(device.createImageView(&createInfo, nullptr, &image_view));
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

		if (format == vk::Format::eD32SfloatS8Uint ||
			format == vk::Format::eD24UnormS8Uint ||
			format == vk::Format::eD32Sfloat ||
			format == vk::Format::eD16Unorm)
		{
			filter = vk::Filter::eNearest;
		}
		else
		{
			filter = vk::Filter::eLinear;
		}
		return filter;
	}

	void Image::create(vk::Device dev, vk::PhysicalDevice& gpu, Texture& texture, vk::ImageUsageFlags usage_flags)
	{
		device = dev;

		format = texture.getFormat();
		faceCount = texture.get_faceCount();
		arrays = texture.getArrayCount();
		mipLevels = texture.get_mipLevels();
		width = texture.get_width();
		height = texture.get_height();

		vk::ImageCreateInfo image_info({}, 
			vk::ImageType::e2D, format, 
			{ width, height, 1 },
			mipLevels, faceCount * arrays,			// remeber that faceCount are also considered to be array layers 
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | usage_flags,
			vk::SharingMode::eExclusive,
			0, nullptr, vk::ImageLayout::eUndefined);

		if (faceCount == 6)
		{
			image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;
		}

		VK_CHECK_RESULT(device.createImage(&image_info, nullptr, &image));

		// allocate memory for this image
		vk::MemoryRequirements mem_req = device.getImageMemoryRequirements(image);

		uint32_t mem_type = Util::findMemoryType(mem_req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, gpu);
		if (mem_type == UINT32_MAX) 
		{
			LOGGER_ERROR("Unable to find required gpu memory type.");
		}
		vk::MemoryAllocateInfo alloc_info(mem_req.size, mem_type);

		VK_CHECK_RESULT(device.allocateMemory(&alloc_info, nullptr, &image_memory));
		device.bindImageMemory(image, image_memory, 0);
	}

	void Image::transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::CommandBuffer& cmdBuff, uint32_t baseMipMapLevel)
	{

		vk::ImageAspectFlags mask = ImageView::getImageAspect(format);

		vk::AccessFlags src_barr, dst_barr;

		switch (old_layout) 
		{
			case vk::ImageLayout::eUndefined:
				src_barr = (vk::AccessFlagBits)0;
				break;
			case vk::ImageLayout::eTransferDstOptimal:
				src_barr = vk::AccessFlagBits::eTransferWrite;
				break;
			case vk::ImageLayout::eTransferSrcOptimal:
				src_barr = vk::AccessFlagBits::eTransferRead;
				break;
			case vk::ImageLayout::eColorAttachmentOptimal:
				src_barr = vk::AccessFlagBits::eColorAttachmentWrite;
				break;
			default:
				src_barr = (vk::AccessFlagBits)0;
		}

		switch (new_layout) 
		{
			case vk::ImageLayout::eTransferDstOptimal:
				dst_barr = vk::AccessFlagBits::eTransferWrite;
				break;
			case vk::ImageLayout::eTransferSrcOptimal:
				dst_barr = vk::AccessFlagBits::eTransferRead;
				break;
			case vk::ImageLayout::eColorAttachmentOptimal:
				dst_barr = vk::AccessFlagBits::eColorAttachmentWrite;
				break;
			case vk::ImageLayout::eShaderReadOnlyOptimal:
				dst_barr = vk::AccessFlagBits::eShaderRead;
				break;
			case vk::ImageLayout::eDepthStencilAttachmentOptimal:
				dst_barr = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				break;
			default:
				dst_barr = (vk::AccessFlagBits)0;
		}

		vk::ImageSubresourceRange subresourceRange(mask, 0, mipLevels, 0, arrays * faceCount);

		if (baseMipMapLevel != UINT32_MAX)
		{
			subresourceRange.baseMipLevel = baseMipMapLevel;
			subresourceRange.levelCount = 1;
		}

		vk::ImageMemoryBarrier mem_barr(src_barr, dst_barr, 
			old_layout, new_layout, 
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
			image,
			subresourceRange);

		cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &mem_barr);
	}

	// image-based functions =======
	void Image::generate_mipmap(vk::CommandBuffer cmdBuffer)
	{
		for (uint8_t i = 1; i < mipLevels; ++i) 
		{
			// source
			vk::ImageSubresourceLayers src(
				vk::ImageAspectFlagBits::eColor,
				i - 1,
				0, 1);
			vk::Offset3D src_offset(
				width >> (i - 1),
				height >> (i - 1),
				1);

			// destination
			vk::ImageSubresourceLayers dst(
				vk::ImageAspectFlagBits::eColor,
				i,
				0, 1);
			vk::Offset3D dst_offset(
				width >> i,
				height >> i,
				1);

			vk::ImageBlit image_blit;
			image_blit.srcSubresource = src;
			image_blit.srcOffsets[1] = src_offset;
			image_blit.dstSubresource = dst;
			image_blit.dstOffsets[1] = dst_offset;

			// create image barrier - transition image to transfer 
			transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmdBuffer, i);

			// blit the image
			cmdBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &image_blit, vk::Filter::eLinear);
			transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, cmdBuffer, i);
		}

		// prepare for shader read
		transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, cmdBuffer);
	}

	void Image::blit(VulkanAPI::Image& other_image, VulkanAPI::Queue& graph_queue)
	{
		// source
		vk::ImageAspectFlags imageAspect = ImageView::getImageAspect(format);

		vk::ImageSubresourceLayers src(
			imageAspect,
			0, 0, 1);
		vk::Offset3D src_offset(
			width, height, 1);

		// destination
		vk::ImageSubresourceLayers dst(
			imageAspect,
			0, 0, 1);
		vk::Offset3D dst_offset(
			width, height, 1);

		vk::ImageBlit image_blit;
		image_blit.srcSubresource = src;
		image_blit.srcOffsets[1] = src_offset;
		image_blit.dstSubresource = dst;
		image_blit.dstOffsets[1] = dst_offset;

		// cmd buffer required for the image blit
		CommandBuffer blit_cmd_buff(device, graph_queue.get_index());
		blit_cmd_buff.createPrimary();

		transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, blit_cmd_buff.get());
		other_image.transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal, blit_cmd_buff.get());

		// blit the image 
		vk::Filter filter = getFilterType(format);
		blit_cmd_buff.get().blitImage(other_image.get(), vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &image_blit, filter);

		transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, blit_cmd_buff.get());
		other_image.transition(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, blit_cmd_buff.get());

		// flush the cmd buffer
		blit_cmd_buff.end();
		graph_queue.flush_cmdBuffer(blit_cmd_buff.get());
	}
}
