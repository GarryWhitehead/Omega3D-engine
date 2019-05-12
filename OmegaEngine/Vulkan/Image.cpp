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

	vk::ImageViewType ImageView::get_texture_type(uint32_t faces, uint32_t array_count)
	{
		if (array_count > 1 && faces == 1)
		{
			return vk::ImageViewType::e2DArray;		//only dealing with 2d textures at the moment
		}
		else if (faces == 6)
		{
			if (array_count == 1)
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

		vk::ImageViewType type = get_texture_type(image.get_faces(), image.get_array_count());

		// making assumptions here based on the image format used
		vk::ImageAspectFlags aspect;

		switch (image.get_format()) 
		{
			case vk::Format::eD32Sfloat:
				[[__fallthrough]]
			case vk::Format::eD32SfloatS8Uint:
				[[ __fallthrough ]]
			case vk::Format::eD24UnormS8Uint:
				aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
				break;
			case vk::Format::eD16Unorm:
				aspect = vk::ImageAspectFlagBits::eDepth;
				break;
			default:
				aspect = vk::ImageAspectFlagBits::eColor;
		}

		vk::ImageViewCreateInfo createInfo({},
			image.get(), type, image.get_format(),
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

	void Image::create(vk::Device dev, vk::PhysicalDevice& gpu, Texture& texture, vk::ImageUsageFlags usage_flags)
	{
		device = dev;

		format = texture.get_format();
		faces = texture.get_faces();
		arrays = texture.get_array_count();
		mip_levels = texture.get_mip_levels();
		width = texture.get_width();
		height = texture.get_height();

		vk::ImageCreateInfo image_info({}, 
			vk::ImageType::e2D, format, 
			{ width, height, 1 },
			mip_levels, 1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | usage_flags,
			vk::SharingMode::eExclusive,
			0, nullptr, vk::ImageLayout::eUndefined);

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

	void Image::transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t levelCount, vk::CommandBuffer& cmdBuff)
	{

		vk::ImageAspectFlags mask;
		if (new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) 
		{
			if (format == vk::Format::eD32SfloatS8Uint|| format == vk::Format::eD24UnormS8Uint) 
			{
				mask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			}
			else 
			{
				mask = vk::ImageAspectFlagBits::eDepth;
			}
		}
		else {
			mask = vk::ImageAspectFlagBits::eColor;
		}

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

		vk::ImageMemoryBarrier mem_barr(src_barr, dst_barr, 
			old_layout, new_layout, 
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
			image,
			vk::ImageSubresourceRange(mask, 0, mip_levels, 0, arrays));

		cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &mem_barr);
	}

	// image-based functions =======
	void Image::generate_mipmap(vk::CommandBuffer cmd_buffer)
	{
		for (uint8_t i = 1; i < mip_levels; ++i) 
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
			vk::ImageMemoryBarrier write_mem_barrier(
				(vk::AccessFlags)0, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				0, 0,
				image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, i, 1, 0, 1));

			cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &write_mem_barrier);

			// blit the image
			cmd_buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &image_blit, vk::Filter::eLinear);

			vk::ImageMemoryBarrier read_mem_barrier(
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
				0, 0,
				image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, i, 1, 0, 1));

			cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &read_mem_barrier);
		}

		// prepare for shader read
		vk::ImageMemoryBarrier read_mem_barrier(
			vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead,
			vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
			0, 0,
			image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1));

		cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &read_mem_barrier);
	}

	void Image::blit(VulkanAPI::Image& other_image, VulkanAPI::Queue& graph_queue, vk::ImageAspectFlagBits aspect_flags)
	{
		// source
		vk::ImageSubresourceLayers src(
			vk::ImageAspectFlagBits::eColor,
			0, 0, 1);
		vk::Offset3D src_offset(
			width, height, 1);

		// destination
		vk::ImageSubresourceLayers dst(
			vk::ImageAspectFlagBits::eColor,
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
		blit_cmd_buff.create_primary();

		// blit the image
		blit_cmd_buff.get().blitImage(other_image.get(), vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &image_blit, vk::Filter::eLinear);

		// flush the cmd buffer
		blit_cmd_buff.end();
		graph_queue.flush_cmd_buffer(blit_cmd_buff.get());
	}
}
