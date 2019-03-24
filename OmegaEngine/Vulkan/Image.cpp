#include "Image.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/Buffer.h"

namespace VulkanAPI
{
	ImageView::ImageView()
	{

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

		vk::ImageViewType type;
		switch (image.textureType()) {
		case TextureType::Normal:
			type = vk::ImageViewType::e2D;
			break;
		case TextureType::Array:
			type = vk::ImageViewType::e2DArray;
			break;
		case TextureType::Cube:
			type = vk::ImageViewType::eCube;
			break;
		case TextureType::CubeArray:
			type = vk::ImageViewType::eCubeArray;
			break;
		default:
			type = vk::ImageViewType::e2D;
		}

		// making assumptions here based on the image format used
		vk::ImageAspectFlags aspect;

		switch (image.format()) {
		case vk::Format::eD32Sfloat:
			aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			break;
		case vk::Format::eD32SfloatS8Uint:
		[[ __fallthrough ]]
		case vk::Format::eD24UnormS8Uint:
			aspect = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			break;
		default:
			aspect = vk::ImageAspectFlagBits::eColor;
		}

		vk::ImageViewCreateInfo createInfo({},
			image.get(), type, image.format(),
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

	void Image::create(vk::Device dev, vk::PhysicalDevice& gpu, vk::Format format, uint32_t width, uint32_t height, vk::ImageUsageFlags usage_flags, TextureType type)
	{
		device = dev;

		image_format = format;
		image_layers = 1;
		image_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1.0);
		image_width = width;
		image_height = height;
		image_type = type;

		vk::ImageCreateInfo image_info({}, vk::ImageType::e2D, format, 
			{ width, height, 1 },
			image_mip_levels, 1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | usage_flags,
			vk::SharingMode::eExclusive,
			0, nullptr, vk::ImageLayout::eUndefined);

		VK_CHECK_RESULT(device.createImage(&image_info, nullptr, &image));

		// allocate memory for this image
		vk::MemoryRequirements mem_req = device.getImageMemoryRequirements(image);

		uint32_t mem_type = Util::findMemoryType(mem_req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, gpu);
		vk::MemoryAllocateInfo alloc_info(mem_req.size, mem_type);

		VK_CHECK_RESULT(device.allocateMemory(&alloc_info, nullptr, &image_memory));
		device.bindImageMemory(image, image_memory, 0);
	}

	void Image::transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t levelCount, vk::CommandBuffer cmdBuff, vk::Queue graphQueue, vk::CommandPool cmdPool)
	{
		vk::CommandBuffer comm_buff;
		if (!cmdBuff) {

			assert(cmdPool);
			assert(graphQueue);
			comm_buff = Util::beginSingleCmdBuffer(cmdPool, device);
		}
		else {
			comm_buff = cmdBuff;
		}

		vk::ImageAspectFlags mask;
		if (new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {

			if (image_format == vk::Format::eD32SfloatS8Uint|| image_format == vk::Format::eD24UnormS8Uint) {

				mask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
			}
			else {
				mask = vk::ImageAspectFlagBits::eDepth;
			}
		}
		else {
			mask = vk::ImageAspectFlagBits::eColor;
		}

		vk::AccessFlags src_barr, dst_barr;

		switch (old_layout) {
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

		switch (new_layout) {
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
			vk::ImageSubresourceRange(mask, 0, static_cast<uint32_t>(image_mip_levels), 0, static_cast<uint32_t>(image_layers)));

		comm_buff.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &mem_barr);

		if (!cmdBuff) {

			Util::submitToQueue(comm_buff, graphQueue, cmdPool, device);
		}
	}

	// image-based functions =======
	void Image::generate_mipmap(vk::CommandBuffer cmd_buffer)
	{
		for (uint8_t i = 0; i < image_mip_levels; ++i) {

			// source
			vk::ImageSubresourceLayers src(
				vk::ImageAspectFlagBits::eColor,
				i - 1,
				0, 0);
			vk::Offset3D src_offset(
				image_width >> (i - 1),
				image_height >> (i - 1),
				1);

			// destination
			vk::ImageSubresourceLayers dst(
				vk::ImageAspectFlagBits::eColor,
				i,
				0, 0);
			vk::Offset3D dst_offset(
				image_width >> i,
				image_height >> i,
				1);

			vk::ImageBlit image_blit;
			image_blit.srcSubresource = src;
			image_blit.srcOffsets[1] = src_offset;
			image_blit.dstSubresource = dst;
			image_blit.dstOffsets[1] = dst_offset;

			// sub range required for barrier
			vk::ImageSubresourceRange mip_subrange(
				vk::ImageAspectFlagBits::eColor,
				i,
				1, 1);

			// create image barrier - transition image to transfer 
			vk::ImageMemoryBarrier write_mem_barrier(
				(vk::AccessFlags)0, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				0, 0,
				image, mip_subrange);

			cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &write_mem_barrier);

			// blit the image
			cmd_buffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &image_blit, vk::Filter::eLinear);

			vk::ImageMemoryBarrier read_mem_barrier(
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
				0, 0,
				image, mip_subrange);

			cmd_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, (vk::DependencyFlags)0, 0, nullptr, 0, nullptr, 1, &read_mem_barrier);
		}
	}
}
