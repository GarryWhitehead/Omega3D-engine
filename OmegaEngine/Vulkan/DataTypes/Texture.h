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
		Texture(vk::Device dev, vk::PhysicalDevice phys);
		~Texture();

		void init(vk::Device dev, vk::PhysicalDevice phys, Queue& queue);
		void init(vk::Device dev, vk::PhysicalDevice phys);

		void createEmptyImage(vk::Format, uint32_t width, uint32_t height, uint8_t mipLevels, vk::ImageUsageFlags usageFlags);
		void map(OmegaEngine::MappedTexture& tex);
		void createCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers);
		void createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers);

		vk::ImageView& getImageView();

		vk::Format& getFormat()
		{
			return format;
		}

		Image& getImage()
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

		uint32_t getFaceCount() const
		{
			return faceCount;
		}

		uint32_t getArrayCount() const
		{
			return arrays;
		}

		uint32_t get_mipLevels() const
		{
			return mipLevels;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		Queue graphicsQueue;

		// texture info
		vk::Format format;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t faceCount = 1;
		uint32_t arrays = 1;
		uint32_t mipLevels = 1;

		Image image;
		ImageView imageView;
	};

}

