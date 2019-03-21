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

		// frame submit and presentation to the swapchain
		void begin_frame(vk::Semaphore& image_semaphore);
		void submit_frame(vk::Semaphore& present_semaphore, vk::Queue& present_queue);

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

		uint32_t get_image_index() const
		{
			return image_index;
		}

	private:

		vk::Device dev;

		vk::SurfaceKHR surface;
		vk::Extent2D extent;
		vk::SurfaceFormatKHR format;
		vk::SwapchainKHR swapchain;

		std::vector<VulkanAPI::ImageView> image_views;

		// current image
		uint32_t image_index = 0;

	};
}

