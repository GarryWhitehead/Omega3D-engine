#include "Texture.h"

#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Interface.h"
#include "DataTypes/TextureType.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Image.h"
#include "DataTypes/TextureType.h"

namespace VulkanAPI
{
	Texture::Texture()
	{

	}

	Texture::Texture(TextureType type) :
		tex_type(type)
	{
	}


	Texture::~Texture()
	{
	}

	void Texture::create_empty_image(vk::Format format, uint32_t width, uint32_t height, uint8_t mip_levels, vk::ImageUsageFlags usage_flags)
	{
		// create an empty image
		tex_image = std::make_unique<Image>(device);
		tex_image->create(format, width, height, mip_levels, 1, TextureType::Normal);

		// and a image view of the empty image
		tex_imageView = std::make_unique<ImageView>(device);
		tex_imageView->create(tex_image);
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

		tex_image = std::make_unique<Image>(device);
		tex_image->create(tex.format(), tex.tex_width(), tex.tex_height(), tex.mipmapCount(), tex.tex_layers(), tex_type);
		vk::MemoryRequirements mem_req = device.getImageMemoryRequirements(tex_image->get());

		uint32_t mem_type = Util::findMemoryType(mem_req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, gpu);
		vk::MemoryAllocateInfo alloc_info(mem_req.size, mem_type);
		
		VK_CHECK_RESULT(device.allocateMemory(&alloc_info, nullptr, &tex_memory));
		device.bindImageMemory(tex_image->get(), tex_memory, 0);

		// create the info required for the copy
		std::vector<vk::BufferImageCopy> copy_buffers;
		if (tex_type == TextureType::Normal) {
			createCopyBuffer(copy_buffers);
		}
		else {
			createArrayCopyBuffer(copy_buffers);
		}
		
		// noew copy image to local device
		vk::CommandBuffer comm_buff = Util::beginSingleCmdBuffer(cmdPool, device);
		tex_image->transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, comm_buff);

		comm_buff.copyBufferToImage(staging_buff, tex_image->get(), vk::ImageLayout::eTransferDstOptimal, static_cast<uint32_t>(copy_buffers.size()), copy_buffers.data());
		tex_image->transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, comm_buff);
		Util::submitToQueue(comm_buff, graphQueue, cmdPool, device);

		// create an image view of the texture image
		tex_imageView = std::make_unique<ImageView>(device);
		tex_imageView->create(tex_image);

		device.destroyBuffer(staging_buff, nullptr);
		device.freeMemory(stagingMemory, nullptr);
	}

	void Texture::createCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers)
	{
		uint32_t offset = 0;
		for (uint32_t level = 0; level < mipLevels; ++level) {

			vk::BufferImageCopy image_copy(0, 0, 0,
				{ vk::ImageAspectFlagBits::eColor, level, 1, 0 },
				{ 0, 0, 0 },
				{ static_cast<uint32_t>(tex.tex_width()), static_cast<uint32_t>(tex.tex_height()), 1 });
			copy_buffers.emplace_back(image_copy);

			offset += static_cast<uint32_t>(tex2d[level].size());
		}
	}

	void Texture::createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers)
	{
		uint32_t offset = 0;
		for (uint32_t layer = 0; layer < layerCount; ++layer) {

			for (uint32_t level = 0; level < mipLevels; ++level) {

				vk::BufferImageCopy image_copy(0, 0, 0,
					{ vk::ImageAspectFlagBits::eColor, level, 1, layer },
					{ 0, 0, 0 },
					{ static_cast<uint32_t>(tex.tex_width()), static_cast<uint32_t>(tex.tex_height()), 1 });
				copy_buffers.emplace_back(image_copy);

				offset += static_cast<uint32_t>(tex2d[layer][level].size());
			}
		}
	}

	vk::ImageView& Texture::get_image_view()
	{
		return tex_imageView->get_imageView();
	}

}
