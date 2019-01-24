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

		void create(Image& image);

		void create(vk::Image& image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type);

		vk::ImageView& get_imageView()
		{
			return image_view;
		}

	private:

		vk::Device device;
		vk::ImageView image_view;

	};

	class Image
	{

	public:

		Image(vk::Device dev);
		~Image();

		void create(vk::Format format, uint32_t width, uint32_t height, TextureType type);
		void transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, int levelCount, vk::CommandBuffer cmdBuff = VK_NULL_HANDLE, vk::Queue graphQueue = VK_NULL_HANDLE, vk::CommandPool cmdPool = VK_NULL_HANDLE);
		void generate_mipmap(vk::CommandBuffer cmd_buffer);

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

		uint32_t image_width;
		uint32_t image_height;
		int image_layers = 1;
		int image_mip_levels = 0;

		TextureType type;
	};

}

