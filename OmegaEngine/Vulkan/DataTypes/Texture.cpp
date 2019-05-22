#include "Texture.h"

#include "Vulkan/Interface.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Managers/DataTypes/TextureType.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Image.h"

namespace VulkanAPI
{
	Texture::Texture()
	{

	}

	Texture::Texture(vk::Device dev, vk::PhysicalDevice phys, Queue& queue) 
	{
		init(dev, phys, queue);
	}

	Texture::Texture(vk::Device dev, vk::PhysicalDevice phys)
	{
		init(dev, phys);
	}

	Texture::~Texture()
	{
	}

	void Texture::init(vk::Device dev, vk::PhysicalDevice phys, Queue& queue)
	{
		device = dev;
		gpu = phys;
		graph_queue = queue;
	}

	void Texture::init(vk::Device dev, vk::PhysicalDevice phys)
	{
		device = dev;
		gpu = phys;
	}

	void Texture::create_empty_image(vk::Format format, uint32_t width, uint32_t height, uint8_t mip_levels, vk::ImageUsageFlags usage_flags)
	{
		assert(device);

		this->format = format;
		this->width = width;
		this->height = height;
		this->mip_levels = mip_levels;

		// create an empty image
		image.create(device, gpu, *this, usage_flags);

		// and a image view of the empty image
		imageView.create(device, image);
	}

	void Texture::map(OmegaEngine::MappedTexture& tex)
	{
		assert(device);

		// store some of the texture attributes locally
		format = tex.get_format();
		width = tex.tex_width();
		height = tex.tex_height();
		mip_levels = tex.mipmapCount();
		faces = tex.get_num_faces();
		arrays = tex.get_array_count();
		
		vk::DeviceMemory stagingMemory;
		vk::Buffer staging_buff;

		Util::createBuffer(device, gpu, tex.get_size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingMemory, staging_buff);

		void *data_dst;
		device.mapMemory(stagingMemory, 0, tex.get_size(), {}, &data_dst);
		memcpy(data_dst, tex.data(), tex.get_size());
		device.unmapMemory(stagingMemory);

		// if generating mip maps, then we need to set the transfer and destination usage flags too
		vk::ImageUsageFlags usage_flags = vk::ImageUsageFlagBits::eSampled;
		vk::ImageLayout final_transition_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		
		if (mip_levels > 1) 
		{
			usage_flags |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
			// if blitting when creating mip-maps, then wwe will transition to shader read after blitting
			final_transition_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		} 

		image.create(device, gpu, *this, usage_flags);
	
		// create the info required for the copy
		std::vector<vk::BufferImageCopy> copy_buffers;
		if (faces == 1 && arrays == 1) 
		{
			createCopyBuffer(copy_buffers);
		}
		else 
		{
			createArrayCopyBuffer(copy_buffers);
		}
		
		// noew copy image to local device - first prepare the image for copying via transitioning to a transfer state. After copying, the image is transistioned ready for reading by the shader
		CommandBuffer copy_cmd_buff(device, graph_queue.get_index());
		copy_cmd_buff.create_primary();

        image.transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, copy_cmd_buff.get());
		copy_cmd_buff.get().copyBufferToImage(staging_buff, image.get(), vk::ImageLayout::eTransferDstOptimal, static_cast<uint32_t>(copy_buffers.size()), copy_buffers.data());
		image.transition(vk::ImageLayout::eTransferDstOptimal, final_transition_layout, copy_cmd_buff.get());
        
		copy_cmd_buff.end();
		graph_queue.flush_cmd_buffer(copy_cmd_buff.get());

		// generate mip maps if required
		if (mip_levels > 1) 
		{
			CommandBuffer blit_cmd_buff(device, graph_queue.get_index());
			blit_cmd_buff.create_primary();

			image.generate_mipmap(blit_cmd_buff.get());

			blit_cmd_buff.end();
			graph_queue.flush_cmd_buffer(blit_cmd_buff.get());
		}

		// create an image view of the texture image
		imageView.create(device, image);

		device.destroyBuffer(staging_buff, nullptr);
		device.freeMemory(stagingMemory, nullptr);
	}

	void Texture::createCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers)
	{
		uint32_t offset = 0;
		for (uint32_t level = 0; level < mip_levels; ++level) 
		{
			vk::BufferImageCopy image_copy(offset, 0, 0,
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, level, 0, 1),
				vk::Offset3D(0, 0, 0),
				vk::Extent3D(width >> level, height >> level, 1));
			copy_buffers.emplace_back(image_copy);

			offset += (width >> level) * (height >> level);
		}
	}

	void Texture::createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers)
	{
		uint32_t offset = 0;
		for (uint32_t face = 0; face < faces; ++face) 
		{
			for (uint32_t level = 0; level < mip_levels; ++level) 
			{

				vk::BufferImageCopy image_copy(offset, 0, 0,
					{ vk::ImageAspectFlagBits::eColor, level, face, 1},
					{ 0, 0, 0 },
					{ width >> level, height >> level, 1 });
				copy_buffers.emplace_back(image_copy);

				offset += (width >> level) * (height >> level);
			}
		}
	}

	vk::ImageView& Texture::get_image_view()
	{
		return imageView.get_imageView();
	}

}
