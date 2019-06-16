#pragma once
#include "VulkanAPI/Common.h"

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
		static vk::ImageViewType getTextureType(uint32_t faceCount, uint32_t arrayCount);

		void create(vk::Device dev, Image& image);

		void create(vk::Device dev, vk::Image& image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type);

		vk::ImageView& getImageView()
		{
			return imageView;
		}

	private:

		vk::Device device;
		vk::ImageView imageView;

	};

	class Image
	{

	public:

		Image();
		~Image();

		static vk::Filter getFilterType(vk::Format format);

		void create(vk::Device dev, vk::PhysicalDevice& gpu, Texture& texture, vk::ImageUsageFlags usageFlags);
		void transition(vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer& cmdBuff, uint32_t baseMipMapLevel = UINT32_MAX);
		void generateMipMap(vk::CommandBuffer cmdBuffer);
		void blit(VulkanAPI::Image& otherImage, VulkanAPI::Queue& graphicsQueue);

		vk::Image& get()
		{
			return image;
		}

		vk::Format& getFormat()
		{
			return format;
		}

		uint32_t getFaceCount() const
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
		vk::DeviceMemory imageMemory;

		uint32_t width;
		uint32_t height;
		uint32_t faceCount = 1;
		uint32_t arrays = 1;
		uint32_t mipLevels = 1;
	};

}

