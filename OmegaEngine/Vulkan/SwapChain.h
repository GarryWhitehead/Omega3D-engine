#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/RenderPass.h"

#include <vector>

namespace VulkanAPI
{
	class Swapchain
	{
	
	public:

		Swapchain();
		~Swapchain();
		
		void create(Device& device, const uint32_t screenWidth, const uint32_t screenHeight);

		// frame submit and presentation to the swapchain
		void begin_frame(vk::Semaphore& image_semaphore);
		void submit_frame(vk::Semaphore& present_semaphore, vk::Queue& present_queue);

		// sets up the renderpass and framebuffers for the swapchain presentation
		void prepare_swapchain_pass();

		vk::SwapchainKHR& get()
		{
			return swapchain;
		}

		vk::Format& get_format()
		{
			return format.format;
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

		RenderPass& get_renderpass()
		{
			return renderpass;
		}

	private:

		vk::Device dev;
		vk::PhysicalDevice gpu;

		vk::SurfaceKHR surface;
		vk::Extent2D extent;
		vk::SurfaceFormatKHR format;
		vk::SwapchainKHR swapchain;

		std::vector<ImageView> image_views;

		// current image
		uint32_t image_index = 0;

		// swapchain presentation renderpass data
		RenderPass renderpass;
		Texture depth_texture;
		std::array<vk::ClearValue, 2> clear_values = {};
	
	};
}

