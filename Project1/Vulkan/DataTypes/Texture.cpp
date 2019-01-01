#include "Texture.h"

#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Interface.h"
#include "DataTypes/TextureType.h"

namespace VulkanAPI
{

	Texture::Texture()
	{
	}


	Texture::~Texture()
	{
	}

	void Texture::map(OmegaEngine::MappedTexture& tex, std::unique_ptr<MemoryAllocator>& mem_alloc)
	{
		vk::DeviceMemory stagingMemory;
		vk::Buffer staging_buff;
		mem_alloc->createBuffer(tex.size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingMemory, staging_buff);

		void *data_dst;
		device.mapMemory(stagingMemory, 0, tex.size(), {}, &data_dst);
		memcpy(data_dst, tex.data(), tex.size());
		device.unmapMemory(stagingMemory);

		vk::ImageCreateInfo image_info({}, vk::ImageType::e2D, tex.format(), { tex.tex_width(), tex.tex_height(), 1 },
		tex.mipmapCount, tex.tex_layers,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,		
		vk::SharingMode::eExclusive,
		0, nullptr, vk::ImageLayout::eUndefined);

		VK_CHECK_RESULT(device.createImage(&image_info, nullptr, &image));

		VkMemoryRequirements mem_req;
		vkGetImageMemoryRequirements(device, image, &mem_req);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_req.size;
		alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice);

		VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, nullptr, &texMemory));

		vkBindImageMemory(device, image, texMemory, 0);

		VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, device, mipLevels, 1);

		// copy image
		VkCommandBuffer comm_buff = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

		std::vector<VkBufferImageCopy> imageCopyBuffers;
		uint32_t offset = 0;

		for (uint32_t level = 0; level < mipLevels; ++level) {

			VkBufferImageCopy image_copy = {};
			image_copy.imageExtent.width = static_cast<uint32_t>(tex2d[level].extent().x);
			image_copy.imageExtent.height = static_cast<uint32_t>(tex2d[level].extent().y);
			image_copy.imageExtent.depth = 1;
			image_copy.bufferOffset = 0;
			image_copy.bufferRowLength = 0;
			image_copy.bufferImageHeight = 0;
			image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_copy.imageSubresource.mipLevel = level;
			image_copy.imageSubresource.layerCount = 1;
			image_copy.imageSubresource.baseArrayLayer = 0;
			image_copy.imageOffset = { 0,0,0 };
			imageCopyBuffers.emplace_back(image_copy);

			offset += static_cast<uint32_t>(tex2d[level].size());
		}

		vkCmdCopyBufferToImage(comm_buff, staging_buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());
		VulkanUtility::SubmitCmdBufferToQueue(comm_buff, graphQueue, cmdPool, device);

		VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, device, mipLevels, 1);

		imageView = VulkanUtility::InitImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, device);

		CreateTextureSampler(addrMode, maxAnisotropy, color, VK_FILTER_LINEAR);

		vkDestroyBuffer(device, staging_buff, nullptr);
		vkFreeMemory(device, stagingMemory, nullptr);
	}
}
