#pragma once
#include "volk.h"
#include "vulkan/vulkan.hpp"
#include "Vulkan/Device.h"
#include <vector>

namespace VulkanAPI
{
	class Swapchain
	{
	
	public:

		Swapchain();
		~Swapchain();
		
		void create(VulkanAPI::Device& device, const uint32_t screenWidth, const uint32_t screenHeight);

	private:

		vk::SurfaceKHR surface;
		vk::Extent2D extent;
		vk::SwapchainKHR swapchain;
		std::vector<vk::ImageView> image_views;
	};
}

