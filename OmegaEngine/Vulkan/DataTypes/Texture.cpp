#include "Texture.h"

#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Interface.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Managers/DataTypes/TextureType.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Image.h"

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

	void Texture::init(vk::Device dev, vk::PhysicalDevice phys, Queue& queue, TextureType type)
	{
		device = dev;
		gpu = phys;
		graph_queue = queue;
		tex_type = type;
	}

	void Texture::create_empty_image(vk::Device& device, vk::PhysicalDevice& gpu, vk::Format format, uint32_t width, uint32_t height, uint8_t mip_levels, vk::ImageUsageFlags usage_flags)
	{
		assert(device);

		// create an empty image
		tex_image.create(device, gpu, format, width, height, usage_flags, TextureType::Normal);

		// and a image view of the empty image
		tex_imageView.create(device, tex_image);
	}

	void Texture::map(OmegaEngine::MappedTexture& tex)
	{
		assert(device);

		// store some of the texture attributes locally
		tex_width = tex.tex_width();
		tex_height = tex.tex_height();
		mip_levels = tex.mipmapCount();
		
		vk::DeviceMemory stagingMemory;
		vk::Buffer staging_buff;
		MemoryAllocator& mem_alloc = Global::Managers::mem_allocator;
		mem_alloc.createBuffer(tex.size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingMemory, staging_buff);

		void *data_dst;
		device.mapMemory(stagingMemory, 0, tex.size(), {}, &data_dst);
		memcpy(data_dst, tex.data(), tex.size());
		device.unmapMemory(stagingMemory);

		tex_image.create(device, gpu, tex.get_format(), tex.tex_width(), tex.tex_height(), vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, tex_type);
	
		// create the info required for the copy
		std::vector<vk::BufferImageCopy> copy_buffers;
		if (tex_type == TextureType::Normal) {
			createCopyBuffer(copy_buffers);
		}
		else {
			createArrayCopyBuffer(copy_buffers);
		}
		
		// noew copy image to local device - first prepare the image for copying via transitioning to a transfer state. After copying, the image is transistioned ready for reading by the shader
		CommandBuffer copy_cmd_buff(device, graph_queue.get_index());
		copy_cmd_buff.create_primary(CommandBuffer::UsageType::Single);

        tex_image.transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, copy_cmd_buff.get(), graph_queue.get(), copy_cmd_buff.get_pool());
		copy_cmd_buff.get().copyBufferToImage(staging_buff, tex_image.get(), vk::ImageLayout::eTransferDstOptimal, static_cast<uint32_t>(copy_buffers.size()), copy_buffers.data());
		tex_image.transition(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, copy_cmd_buff.get(), graph_queue.get(), copy_cmd_buff.get_pool());
        
		copy_cmd_buff.end();
		graph_queue.flush_cmd_buffer(copy_cmd_buff.get());

		// generate mip maps if required
		CommandBuffer blit_cmd_buff(device, graph_queue.get_index());
		blit_cmd_buff.create_primary(CommandBuffer::UsageType::Single);

		if (mip_levels > 1) {
			tex_image.generate_mipmap(blit_cmd_buff.get());
		}

		blit_cmd_buff.end();
		graph_queue.flush_cmd_buffer(blit_cmd_buff.get());

		// create an image view of the texture image
		tex_imageView.create(device, tex_image);

		device.destroyBuffer(staging_buff, nullptr);
		device.freeMemory(stagingMemory, nullptr);
	}

	void Texture::createCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers)
	{
		uint32_t offset = 0;
		for (uint32_t level = 0; level < mip_levels; ++level) {

			vk::BufferImageCopy image_copy(offset, 0, 0,
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, level, 0, 1),
				vk::Offset3D(0, 0, 0),
				vk::Extent3D(tex_width >> level, tex_height >> level, 1));
			copy_buffers.emplace_back(image_copy);

			offset += (tex_width >> level) * (tex_height >> level);
		}
	}

	void Texture::createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers)
	{
		uint32_t offset = 0;
		for (uint32_t layer = 0; layer < tex_layers; ++layer) {

			for (uint32_t level = 0; level < mip_levels; ++level) {

				vk::BufferImageCopy image_copy(offset, 0, 0,
					{ vk::ImageAspectFlagBits::eColor, level, 1, layer },
					{ 0, 0, 0 },
					{ tex_width, tex_height, 1 });
				copy_buffers.emplace_back(image_copy);


				offset += (tex_width >> level) * (tex_height >> level);
			}
		}
	}

	vk::ImageView& Texture::get_image_view()
	{
		return tex_imageView.get_imageView();
	}

}
