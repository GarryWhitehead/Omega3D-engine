#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include <vector>

namespace VulkanAPI
{
	class Swapchain
	{
	
	public:

		Swapchain();
		~Swapchain();
		
		void create(VulkanAPI::Device& device, const uint32_t screenWidth, const uint32_t screenHeight);

		vk::SwapchainKHR& get()
		{
			return swapchain;
		}

		vk::Format& get_format()
		{
			return format.format;
		}

		vk::ImageView& get_image_view(uint32_t index)
		{
			return image_views[index].get_imageView();
		}

		uint32_t get_image_count() const
		{
			return image_views.size();
		}

		uint32_t get_extents_height() const
		{
			return extent.height;
		}

		uint32_t get_extents_width() const
		{
			return extent.width;
		}

	private:

		vk::Device dev;

		vk::SurfaceKHR surface;
		vk::Extent2D extent;
		vk::SurfaceFormatKHR format;
		vk::SwapchainKHR swapchain;
		std::vector<VulkanAPI::ImageView> image_views;
	};
}

