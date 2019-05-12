#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/Queue.h"
#include "Vulkan/Image.h"

namespace OmegaEngine
{
	class MappedTexture;
}

namespace VulkanAPI
{
	// forward declerations
	class MemoryAllocator;
	class Image;
	class ImageView;

	class Texture 
	{

	public:

		Texture();
		Texture(vk::Device dev, vk::PhysicalDevice phys, Queue& queue);
		~Texture();

		void init(vk::Device dev, vk::PhysicalDevice phys, Queue& queue);
		void init(vk::Device dev, vk::PhysicalDevice phys);

		void create_empty_image(vk::Format, uint32_t width, uint32_t height, uint8_t mip_levels, vk::ImageUsageFlags usage_flags);
		void map(OmegaEngine::MappedTexture& tex);
		void createCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers);
		void createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers);

		vk::ImageView& get_image_view();

		vk::Format& get_format()
		{
			return format;
		}

		Image& get_image()
		{
			return image;
		}

		uint32_t get_width() const
		{
			return width;
		}

		uint32_t get_height() const
		{
			return height;
		}

		uint32_t get_faces() const
		{
			return faces;
		}

		uint32_t get_array_count() const
		{
			return arrays;
		}

		uint32_t get_mip_levels() const
		{
			return mip_levels;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		Queue graph_queue;

		// texture info
		vk::Format format;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t faces = 1;
		uint32_t arrays = 1;
		uint32_t mip_levels = 1;

		Image image;
		ImageView imageView;
	};

}

