#pragma once
#include "volk.h"
#include "vulkan/vulkan.hpp"
#include <vector>

namespace VulkanAPI
{
	class SwapchainKHR
	{
	
	public:

		SwapchainKHR(vk::Device device,
			vk::PhysicalDevice gpu,
			vk::Instance instance,
			int graphIndex,
			int presentIndex,
			uint32_t screenWidth,
			uint32_t screenHeight);

		~SwapchainKHR();

	private:

		vk::SurfaceKHR surface;
		vk::Extent2D extent;
		vk::SwapchainKHR swapchain;
		std::vector<vk::ImageView> image_views;
	};
}

