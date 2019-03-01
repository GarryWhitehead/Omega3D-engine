#pragma once
#include "Vulkan/Common.h"

#include <cstdint>

namespace VulkanAPI
{
	// forward decleartions
	enum class TextureType;
	class Image;

	class ImageView
	{
	public:

		ImageView();

		void create(vk::Device dev, Image& image);

		void create(vk::Device dev, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type);

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

		Image();
		~Image();

		void create(vk::Device dev, vk::PhysicalDevice& gpu, vk::Format format, uint32_t width, uint32_t height, vk::ImageUsageFlags usage_flags, TextureType type);
		void transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t levelCount, vk::CommandBuffer cmdBuff, vk::Queue graphQueue, vk::CommandPool cmdPool);
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
		vk::DeviceMemory image_memory;

		uint32_t image_width;
		uint32_t image_height;
		int image_layers = 1;
		int image_mip_levels = 0;

		TextureType type;
	};

}

