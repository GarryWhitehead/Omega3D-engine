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

		static vk::ImageAspectFlags getImageAspect(vk::Format format);
		static vk::ImageViewType getTexture_type(uint32_t faceCount, uint32_t arrayCount);

		void create(vk::Device dev, Image& image);

		void create(vk::Device dev, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type);

		vk::ImageView& getImageView()
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

		static vk::Filter getFilterType(vk::Format format);

		void create(vk::Device dev, vk::PhysicalDevice& gpu, Texture& texture, vk::ImageUsageFlags usage_flags);
		void transition(vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::CommandBuffer& cmdBuff, uint32_t baseMipMapLevel = UINT32_MAX);
		void generate_mipmap(vk::CommandBuffer cmdBuffer);
		void blit(VulkanAPI::Image& other_image, VulkanAPI::Queue& graph_queue);

		vk::Image& get()
		{
			return image;
		}

		vk::Format& getFormat()
		{
			return format;
		}

		uint32_t get_faceCount() const
		{
			return faceCount;
		}

		uint32_t getArrayCount() const
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
		uint32_t faceCount = 1;
		uint32_t arrays = 1;
		uint32_t mipLevels = 1;
	};

}

