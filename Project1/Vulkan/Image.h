#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	// forward decleartions
	enum class TextureType;

	class ImageView
	{
	public:

		ImageView(vk::Device dev);

		void create(std::unique_ptr<Image>& image);

	private:

		vk::Device device;

		vk::ImageView image_view;

	};

	class Image
	{

	public:

		Image(vk::Device dev);
		~Image();

		void create(vk::Format format, uint32_t width, uint32_t height, uint32_t mipmapCount, uint32_t layers, TextureType type);
		void transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, int levelCount, vk::CommandBuffer cmdBuff = VK_NULL_HANDLE, vk::Queue graphQueue = VK_NULL_HANDLE, vk::CommandPool cmdPool = VK_NULL_HANDLE);

		vk::Image& get()
		{
			return image;
		}

		vk::Format& format()
		{
			return image_format;
		}

		TextureType textureType() const
		{
			return type;
		}

	private:

		vk::Device device;

		vk::Image image;
		vk::Format image_format;
		int image_layers = 1;
		int image_mipLevels = 0;
		TextureType type;
	};

}

