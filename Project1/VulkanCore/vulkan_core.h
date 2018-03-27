#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm.hpp"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "stb_image.h"

#include "VulkanCore/vulkan_validation.h"
#include <vector>

class ValidationLayers;

struct QueueInfo
{
	QueueInfo() : graphIndex(-1), presentIndex(-1) {}

	VkQueue graphQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;
	int32_t graphIndex;
	int32_t presentIndex;
	int32_t computeIndex;
};

struct DeviceInfo
{
	VkPhysicalDevice physDevice;
	VkPhysicalDeviceFeatures features;
	VkDevice device;
	uint32_t count;
	std::vector<const char*> deviceExt;
};

struct InstanceExtInfo
{
	InstanceExtInfo() : count(0) {}

	uint32_t count;
	std::vector<const char*> extensions;
};

struct SemaphoreInfo
{
	VkSemaphore image;
	VkSemaphore render;
};

struct SwapChainInfo
{
	VkSwapchainKHR swapChain;
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> modes;
	std::vector<VkImage> images;
	uint32_t imageCount;
};

struct SurfaceInfo
{
	VkSurfaceKHR surface;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR mode;
	VkExtent2D extent;
};

struct ImageViewInfo
{
	std::vector<VkImageView> images;
};


class VulkanCore
{
public:

	VulkanCore(GLFWwindow *window);

	void InitVulkanCore();
	void Release();
	void CreateInstance();
	void InitPhysicalDevice();
	void InitQueues();
	void InitDevice();
	VkSemaphore CreateSemaphore();
	bool FindDeviceExtenisions(VkPhysicalDevice physDevice, const char* reqDevice);
	VkFence CreateFence(VkFenceCreateFlags flags);
	void InitWindowSurface();
	void InitSwapChain();
	VkImageView InitImageView(VkImage image, VkFormat format, VkImageAspectFlagBits imageAspect, VkImageViewType type);

	friend class CommandBuffers;

protected:

	GLFWwindow *m_window;
	int m_screenWidth, m_screenHeight;

	VkInstance m_instance;
	QueueInfo m_queue;
	DeviceInfo m_device;
	InstanceExtInfo m_instanceExt;
	SemaphoreInfo m_semaphore;

	SwapChainInfo m_swapchain;
	ImageViewInfo m_imageView;
	SurfaceInfo m_surface;

	// pointers to critical vulkan components
	ValidationLayers *pValidation;
};