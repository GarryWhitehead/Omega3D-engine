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

		Texture(TextureType type);
		~Texture();

		void map(OmegaEngine::MappedTexture& tex, std::unique_ptr<MemoryAllocator>& mem_alloc);
		void createCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers);
		void createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copy_buffers);

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		TextureType tex_type;
		std::unique_ptr<Image> tex_image;
		std::unique_ptr<ImageView> tex_imageView;
		vk::DeviceMemory tex_memory;
	};

}

