#include "Vulkan_Global.h"

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VkSwapChain.h"
#include "VkMemoryManager.h"

namespace VulkanGlobal
{
	void init_vkInstance()
	{
		vkCurrent.instance = VkInitilisation::CreateInstance();
	}

	void init_windowSurface(GLFWwindow* window)
	{
		assert(vkCurrent.instance != VK_NULL_HANDLE);
		vkCurrent.surface = VkInitilisation::PrepareWindowSurface(vkCurrent.instance, window);
	}

	void init_device()
	{
		// prepare the actual physical device i.e find a vulkan supporting GPU
		vkCurrent.physicalDevice = VkInitilisation::PreparePhysicalDevice(vkCurrent.instance);

		// and get all the features that we can use
		vkCurrent.features = VkInitilisation::getPhysicalDeviceFeatures(vkCurrent.physicalDevice);

		// now init the device and the queues
		VkInitilisation::PrepareQueueIndices(vkCurrent.surface, vkCurrent.physicalDevice, graphIndex, presentIndex, computeIndex);
		vkCurrent.device = VkInitilisation::PrepareDevice(vkCurrent.physicalDevice, graphIndex, presentIndex, computeIndex);
		VkInitilisation::PrepareQueues(vkCurrent.device, graphIndex, presentIndex, computeIndex, vkCurrent.graphQueue, vkCurrent.presentQueue, vkCurrent.computeQueue);
	}

	void init_swapchain(uint32_t width, uint32_t height)
	{
		// the swap chain will hold our image views for presentation
		std::vector<VkImage> swapchainImages = VkInitilisation::PrepareSwapchain(vkCurrent.device, 
			vkCurrent.physicalDevice,
			vkCurrent.instance, 
			vkCurrent.surface, 
			vkCurrent.graphIndex,
			vkCurrent.presentIndex, 
			vkCurrent.format, 
			vkCurrent.mode, 
			vkCurrent.extent, 
			vkCurrent.swapchain,
			width, height);

		// the image views are what we will be using to draw our final screen too - triple buffered if supported
		vkCurrent.imageViews = VkInitilisation::PrepareImageViews(vkCurrent.device, swapchainImages, vkCurrent.format);

		// also create the semaphores which will be used for syncing the presentation to the swapchain
		VkInitilisation::PrepareSemaphores(vkCurrent.device, vkCurrent.imageSemaphore, vkCurrent.presentSemaphore);
	}

	// vulkan management functions
	void init_vkMemoryManager()
	{
		vkManagers.vkMemoryManager = new VkMemoryManager;
		assert(vkManagers.vkMemoryManager != nullptr);
	}
}