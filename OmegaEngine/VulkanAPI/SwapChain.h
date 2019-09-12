#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/RenderPass.h"

#include <vector>

namespace VulkanAPI
{

// forward declerations
class Device;
struct NativeWindowWrapper;

class Swapchain
{

public:
	Swapchain();
	~Swapchain();
	
	// static functions
	// Create a vulkan surface object from a native window pointer
	static SurfaceWrapper createSurface(NativeWindowWrapper& window);

	void create(vk::Device dev, vk::PhysicalDevice &physicalDevice, vk::SurfaceKHR &surface,
	            const uint32_t graphIndex, const uint32_t presentIndex, const uint32_t screenWidth,
	            const uint32_t screenHeight);

	// frame submit and presentation to the swapchain
	void begin_frame(vk::Semaphore &image_semaphore);
	void submitFrame(vk::Semaphore &presentSemaphore, vk::Queue &presentionQueue);

	// sets up the renderpass and framebuffers for the swapchain presentation
	void prepareSwapchainPass();

	vk::SwapchainKHR &get()
	{
		return swapchain;
	}

	vk::Format &getSurfaceFormat()
	{
		return surfaceFormat.format;
	}

	uint32_t getImageCount() const
	{
		return static_cast<uint32_t>(imageViews.size());
	}

	uint32_t getExtentsHeight() const
	{
		return extent.height;
	}

	uint32_t getExtentsWidth() const
	{
		return extent.width;
	}

	uint32_t getImageIndex() const
	{
		return imageIndex;
	}

	RenderPass &getRenderpass()
	{
		return *renderpass;
	}

private:
	vk::Device device;
	vk::PhysicalDevice gpu;

	vk::Extent2D extent;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::SwapchainKHR swapchain;

	std::vector<ImageView> imageViews;

	// current image
	uint32_t imageIndex = 0;

	// swapchain presentation renderpass data
	std::unique_ptr<RenderPass> renderpass;
	std::unique_ptr<Texture> depthTexture;
	std::array<vk::ClearValue, 2> clearValues = {};
};
} // namespace VulkanAPI
