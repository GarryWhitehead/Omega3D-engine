#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/Image.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/RenderPass.h"

#include <vector>

namespace VulkanAPI
{
	class Device;

	class Swapchain
	{
	
	public:

		Swapchain();
		~Swapchain();
		
		void create(vk::Device dev, 
					vk::PhysicalDevice& phys_dev, 
					vk::SurfaceKHR& surface, 
					const uint32_t graph_index, const uint32_t present_index, 
					const uint32_t screen_width, const uint32_t screen_height);

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
			return static_cast<uint32_t>(image_views.size());
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
			return *renderpass;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		vk::SurfaceKHR surface;
		vk::Extent2D extent;
		vk::SurfaceFormatKHR format;
		vk::SwapchainKHR swapchain;

		std::vector<ImageView> image_views;

		// current image
		uint32_t image_index = 0;

		// swapchain presentation renderpass data
		std::unique_ptr<RenderPass> renderpass;
		std::unique_ptr<Texture> depth_texture;
		std::array<vk::ClearValue, 2> clear_values = {};
	
	};
}

