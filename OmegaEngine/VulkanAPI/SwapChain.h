#pragma once

#include "Types/NativeWindowWrapper.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkTexture.h"

#include "VulkanAPI/Platform/Surface.h"

#include <vector>

namespace VulkanAPI
{

// forward declerations
class VkDriver;

class Swapchain
{

public:
	Swapchain();
	~Swapchain();

	// both copyable and moveable
	Swapchain(const Swapchain&) = default;
	Swapchain& operator=(const Swapchain&) = default;
	Swapchain(Swapchain&&) = default;
	Swapchain& operator=(Swapchain&&) = default;

	// static functions
	/**
	* @brief Creates a KHR surface object using a native window pointer.
	* Note: The native window is obtained from a source such as glfw (used in the application for now)
	* @param window A wrapper containg all the info needed to create a surface and swapchain
	* @param instance A vulkan instance wrapper obtained from calling **createInstance**
	*/
	static Platform::SurfaceWrapper createSurface(OmegaEngine::NativeWindowWrapper& window, vk::Instance& instance);

	/**
	* @brief Creates the swapchain using the supplied native window surface
	* Note: The surface is obtained by calling **createSurface**.
	* Note: This function must be called before using with a **Scene** object.
	* @param context A prepared vulkan device object
	* @param surface A surface object for the given platform
	*/
	bool prepare(VkContext& context, Platform::SurfaceWrapper& surface);

	/// creates the image views for the swapchain
	static std::vector<ImageView> prepareImageViews(Swapchain& swapchain, VkContext& context,
	                                                const vk::SurfaceFormatKHR& surfaceFormat);

	vk::SwapchainKHR& get()
	{
		return swapchain;
	}

	uint32_t getExtentsHeight() const
	{
		return extent.height;
	}

	uint32_t getExtentsWidth() const
	{
		return extent.width;
	}

	friend class VkDriver;

private:
	// the dimensions of the current swapchain
	vk::Extent2D extent;

	// a swapchain based on the present surface type
	vk::SwapchainKHR swapchain;

	// Use for syncing the end of the last frame submit, with the beginning of the next
	// This stops data being submitted to the buffer when it hasn't yet been displayed, stopping artifacts
	vk::Semaphore imageWait;

	// Used for syncing the rendering part with the final present, to enure we don't try and present work that
	// hasn't completed
	vk::Semaphore renderComplete;

	// The current frame image index
	uint32_t imageIndex;
};
}    // namespace VulkanAPI
