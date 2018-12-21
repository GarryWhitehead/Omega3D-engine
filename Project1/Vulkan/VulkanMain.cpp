#include "VulkanMain.h"
#include "Vulkan/Vulkan_Global.h"

namespace Vulkan
{
	
	VulkanMain::VulkanMain(GLFWwindow *window, uint32_t width, uint32_t height)
	{
		// WE are using volk, so we need to init this first before creating a vulkan instance
		VK_CHECK_RESULT(volkInitialize());

		// create a new instance of vulkan
		VulkanGlobal::init_vkInstance();

		// init this instance of vulkan with volk
		volkLoadInstance(VulkanGlobal::vkCurrent.instance);

		VulkanGlobal::init_windowSurface(window);			// prepare KHR surface

		// init new physical device, queues for graphics, presentation and compute
		VulkanGlobal::init_device();

		// prepare swap chain and attached image views - so we have something to render too
		VulkanGlobal::init_swapchain(width, height);

		// now we have init the current vulkan device, init all managers that will be used
		VulkanGlobal::init_vkMemoryManager();
	}


	VulkanMain::~VulkanMain()
	{
	}

}
