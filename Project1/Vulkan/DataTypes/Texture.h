#pragma once
#include "Vulkan/Common.h"

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

	enum class TextureType
	{
		Normal,
		Array,
		Cube,
		CubeArray
	};

	class Texture 
	{

	public:

		Texture();
		Texture(TextureType type);
		~Texture();

		void create_empty_image(vk::Format, uint32_t width, uint32_t height, uint8_t mip_levels, vk::ImageUsageFlags usage_flags);
		void map(OmegaEngine::MappedTexture& tex, std::unique_ptr<MemoryAllocator>& mem_alloc);
		void createCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers);
		void createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers);

		vk::ImageView& get_image_view();

		vk::Format& format()
		{
			return tex_format;
		}

		TextureType type() const
		{
			return tex_type;
		}

		void setType(const TextureType type)
		{
			tex_type = type;
		}

		Image& get_image()
		{
			return *tex_image;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		// texture info
		TextureType tex_type;
		vk::Format tex_format;
		uint32_t tex_width = 0;
		uint32_t tex_height = 0;
		uint8_t tex_layers = 0;

		std::unique_ptr<Image> tex_image;
		std::unique_ptr<ImageView> tex_imageView;
		vk::DeviceMemory tex_memory;
	};

}

