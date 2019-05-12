#pragma once
#include "Vulkan/Common.h"

#include <cstdint>

namespace VulkanAPI
{
	// forward decleartions
	class Image;
	class Queue;
	class Texture;

	class ImageView
	{
	public:

		ImageView();

		static vk::ImageViewType get_texture_type(uint32_t faces, uint32_t array_count);

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

		void create(vk::Device dev, vk::PhysicalDevice& gpu, Texture& texture, vk::ImageUsageFlags usage_flags);
		void transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t levelCount, vk::CommandBuffer& cmdBuff);
		void generate_mipmap(vk::CommandBuffer cmd_buffer);
		void blit(VulkanAPI::Image& other_image, VulkanAPI::Queue& graph_queue, vk::ImageAspectFlagBits aspect_flags);

		vk::Image& get()
		{
			return image;
		}

		vk::Format& get_format()
		{
			return format;
		}

		uint32_t get_faces() const
		{
			return faces;
		}

		uint32_t get_array_count() const
		{
			return arrays;
		}

	private:

		vk::Device device;

		vk::Image image;
		vk::Format format;
		vk::DeviceMemory image_memory;

		uint32_t width;
		uint32_t height;
		uint32_t faces = 1;
		uint32_t arrays = 1;
		uint32_t mip_levels = 1;
	};

}

