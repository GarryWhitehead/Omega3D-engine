#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

// forward declerations
class VulkanDevice;
struct GLFWwindow;

class VkSwapChain
{
public:

	struct SurfaceInfo
	{
		VkSurfaceFormatKHR format;
		VkPresentModeKHR mode;
		VkExtent2D extent;
	} surfaceInfo;

	struct SemaphoreInfo
	{
		VkSemaphore image;
		VkSemaphore render;
	} semaphore;

	VkSwapChain(VkSurfaceKHR surf, VkDevice device, GLFWwindow *win);
	~VkSwapChain();

	void Init(VulkanDevice *p_vkDevice, VkInstance instance, uint32_t screenWidth, uint32_t screenHeight);
	void PrepareSwapChain(VulkanDevice *p_vkDevice, VkInstance instance, uint32_t screenWidth, uint32_t screenHeight);
	void PrepareImageViews();
	void PrepareSemaphores();
	void Destroy();

	// swapchain data
	VkSwapchainKHR swapChain;
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> modes;
	std::vector<VkImage> images;
	uint32_t imageCount;

	// surface which is alocated elsewhere
	VkSurfaceKHR surface;

	// image views
	std::vector<VkImageView> imageViews;

private:

	VkDevice device;			// device handle is only available to public via the engine

	GLFWwindow *window;			// keep a local refernce to the current glfw window
};

