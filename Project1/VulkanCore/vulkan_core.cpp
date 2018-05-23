#include "VulkanCore/vulkan_core.h"
#include "VulkanCore/vulkan_utility.h"
#include "VulkanCore/vulkan_validation.h"
#include "VulkanCore/vulkan_tools.h"
#include "utility/file_log.h"
#include <assert.h>
#include <set>
#include <math.h>
#include <algorithm>

VulkanCore::VulkanCore(GLFWwindow *window)
{
	pValidation = new ValidationLayers();

	m_window = window;
}


void glfw_error_callback(int error, const char* description)
{
	g_filelog->WriteLog("GLFW ERROR: code " + error);
	g_filelog->WriteLog("\n Description: ");
	g_filelog->WriteLog(description);
}

void VulkanCore::InitVulkanCore()
{

	this->CreateInstance();
	this->InitWindowSurface();
	this->InitPhysicalDevice();
	this->InitQueues();
	this->InitDevice();

	m_semaphore.image = this->CreateSemaphore();
	m_semaphore.render = this->CreateSemaphore();

	this->InitSwapChain();

	for (int c = 0; c < m_swapchain.images.size(); ++c)
	{
		VkImageView imageView = this->InitImageView(m_swapchain.images[c], m_surface.format.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);
		m_imageView.images.push_back(imageView);
	}
}

void VulkanCore::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "OmegaOne";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	const char **glfwExtension;
	glfwExtension = glfwGetRequiredInstanceExtensions(&m_instanceExt.count);

	for (int c = 0; c < m_instanceExt.count; ++c)
		m_instanceExt.extensions.push_back(glfwExtension[c]);

	if (EnableValidationLayers)
		m_instanceExt.extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	
	// check that the required layer validation is supported before continuing
	if (!pValidation->FindValidationLayers() && EnableValidationLayers)
	{
		*g_filelog << "Critical error! Unable to initialise required validation layers.";
		exit(EXIT_FAILURE);
	}

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = m_instanceExt.count;
	createInfo.ppEnabledExtensionNames = m_instanceExt.extensions.data();

	if (EnableValidationLayers)
	{
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = pValidation->mReqLayers;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_instance));
}

void VulkanCore::InitPhysicalDevice()
{
	vkEnumeratePhysicalDevices(m_instance, &m_device.count, nullptr);
	std::vector<VkPhysicalDevice> devices(m_device.count);
	vkEnumeratePhysicalDevices(m_instance, &m_device.count, devices.data());

	for (auto const& dev : devices)
	{
		if (dev)
		{
			m_device.physDevice = dev;
			break;
		}
	}

	if (m_device.physDevice == VK_NULL_HANDLE)
	{
		g_filelog->WriteLog("Critcal error! No Vulkan supported GPU devices were found.");
		exit(EXIT_FAILURE);
	}
}

void VulkanCore::InitQueues()
{
	uint32_t queueCount = 0;
	VkBool32 presentQueue = false;
	vkGetPhysicalDeviceQueueFamilyProperties(m_device.physDevice, &queueCount, nullptr);
	std::vector<VkQueueFamilyProperties> queues(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_device.physDevice, &queueCount, queues.data());
	
	for (uint32_t c = 0; c < queues.size(); ++c)
	{
		if (queues[c].queueCount > 0 && queues[c].queueFlags & VK_QUEUE_GRAPHICS_BIT) {									// graphics queue?
			m_queue.graphIndex = c;
		}

		if (queues[c].queueCount > 0 && queues[c].queueFlags & VK_QUEUE_COMPUTE_BIT) {									// compute queue?
			m_queue.computeIndex = c;
		}

		vkGetPhysicalDeviceSurfaceSupportKHR(m_device.physDevice, c, m_surface.surface, &presentQueue);
		if (queues[c].queueCount > 0 && presentQueue) {																	// presentation queue?
			m_queue.presentIndex = c;
		}

		if (m_queue.graphIndex > 0 && m_queue.presentIndex > 0) {
			break;
		}
	}

	if (m_queue.graphIndex < 0 || m_queue.presentIndex < 0)
	{
		g_filelog->WriteLog("Critcal error! Required queues not found.");
		exit(EXIT_FAILURE);
	}
	else if (m_queue.computeIndex < 0) {						// The preference is a sepearte compute queue, though if not found, use the graphics queue for compute shaders
		m_queue.computeIndex = m_queue.graphIndex;
	}
}

void VulkanCore::InitDevice()
{
	float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueInfo = {};
	std::set<int> uniqueQueues = { m_queue.graphIndex, m_queue.presentIndex };

	for (auto& queue : uniqueQueues) {
		VkDeviceQueueCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.queueFamilyIndex = queue;
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = &queuePriority;
		queueInfo.push_back(createInfo);
	}

	VkPhysicalDeviceFeatures devFeatures = {};
	devFeatures.samplerAnisotropy = VK_TRUE;
	devFeatures.tessellationShader = VK_TRUE;
	devFeatures.textureCompressionBC = VK_TRUE;
	devFeatures.geometryShader = VK_TRUE;
	devFeatures.shaderStorageImageExtendedFormats = VK_TRUE;

	const std::vector<const char*> swapChainExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	if (!FindDeviceExtenisions(m_device.physDevice, swapChainExt[0])) {
		g_filelog->WriteLog("Critical error! Swap chain extension not found.");
		exit(EXIT_FAILURE);
	}

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueInfo.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfo.size());
	createInfo.pEnabledFeatures = &devFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(swapChainExt.size());
	createInfo.ppEnabledExtensionNames = swapChainExt.data();

	if (EnableValidationLayers) {
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = pValidation->mReqLayers;
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	VK_CHECK_RESULT(vkCreateDevice(m_device.physDevice, &createInfo, nullptr, &m_device.device));

	// Get the feature assocated with the physical device
	vkGetPhysicalDeviceFeatures(m_device.physDevice, &m_device.features);

	vkGetDeviceQueue(m_device.device, m_queue.computeIndex, 0, &m_queue.computeQueue);
	vkGetDeviceQueue(m_device.device, m_queue.graphIndex, 0, &m_queue.graphQueue);
	vkGetDeviceQueue(m_device.device, m_queue.presentIndex, 0, &m_queue.presentQueue);
}

VkSemaphore VulkanCore::CreateSemaphore()
{
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK_RESULT(vkCreateSemaphore(m_device.device, &semaphore_info, nullptr, &semaphore));

	return semaphore;
}

VkFence VulkanCore::CreateFence(VkFenceCreateFlags flags)
{
	VkFence fence;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.flags = flags;

	VK_CHECK_RESULT(vkCreateFence(m_device.device, &createInfo, nullptr, &fence));
	return fence;
}

bool VulkanCore::FindDeviceExtenisions(VkPhysicalDevice physDevice, const char* reqDevice)
{
	uint32_t count;
	
	// Also get all the device extensions for querying later
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &count, nullptr);
	std::vector<VkExtensionProperties> extensions(count);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &count, extensions.data());

	bool success = false;
	for (const auto& ext : extensions) {
		if (!strcmp(ext.extensionName, reqDevice)) {
			success = true;
		}
	}
	return success;
}


void VulkanCore::InitWindowSurface()
{
	VK_CHECK_RESULT(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface.surface));
}

void VulkanCore::InitSwapChain()
{
	// Get the basic surface properties of the physical device
	uint32_t surfaceCount;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device.physDevice, m_surface.surface, &m_swapchain.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.physDevice, m_surface.surface, &surfaceCount, nullptr);

	m_swapchain.formats.resize(surfaceCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_device.physDevice, m_surface.surface, &surfaceCount, m_swapchain.formats.data());

	// And then get the presentation modes available for this device
	uint32_t presentCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.physDevice, m_surface.surface, &presentCount, nullptr);

	m_swapchain.modes.resize(presentCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_device.physDevice, m_surface.surface, &presentCount, m_swapchain.modes.data());

	// make sure that we have suitable swap chain extensions available before continuing
	if (m_swapchain.formats.empty() || m_swapchain.modes.empty())
	{
		g_filelog->WriteLog("Critcal error! Unable to locate suitable swap chains on device.");
		exit(EXIT_FAILURE);
	}

	// Next step is to determine the surface format. Ideally undefined format is preffered so we can set our own, otherwise
	// we will go with one that suits our colour needs - i.e. 8bitBGRA and SRGB.
	if ((m_swapchain.formats.size() > 0) && (m_swapchain.formats[0].format == VK_FORMAT_UNDEFINED))
	{
		m_surface.format.format = VK_FORMAT_B8G8R8A8_UNORM;
		m_surface.format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	else
	{
		for (auto& format : m_swapchain.formats)
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				m_surface.format = format;
				break;
			}
	}

	// And then the presentation format - the preferred format is triple buffering
	for (auto& mode : m_swapchain.modes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)													// our preferred triple buffering mode
		{
			m_surface.mode = mode;
			break;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			m_surface.mode = mode;																	// immediate mode is only supported on some drivers.
			break;
		}

	// Finally set the resoultion of the swap chain buffers
	// First of check if we can manually set the dimension - some GPUs allow this by setting the max as the size of uint32
	if (m_swapchain.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		m_surface.extent = m_swapchain.capabilities.currentExtent;									// go with the automatic settings
	else
	{
		m_surface.extent = { static_cast<uint32_t>(m_screenWidth), static_cast<uint32_t>(m_screenHeight) };
		m_surface.extent.width = std::max(m_swapchain.capabilities.minImageExtent.width, std::min(m_swapchain.capabilities.maxImageExtent.width, m_surface.extent.width));
		m_surface.extent.height = std::max(m_swapchain.capabilities.minImageExtent.height, std::min(m_swapchain.capabilities.maxImageExtent.height, m_surface.extent.height));
	}

	// Get the number of possible images we can send to the queue
	m_swapchain.imageCount = m_swapchain.capabilities.minImageCount + 1;								 // adding one as we would like to implement triple buffering
	if (m_swapchain.capabilities.maxImageCount > 0 && m_swapchain.imageCount > m_swapchain.capabilities.maxImageCount)
		m_swapchain.imageCount = m_swapchain.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface.surface;
	createInfo.minImageCount = m_swapchain.imageCount;
	createInfo.imageFormat = m_surface.format.format;
	createInfo.imageColorSpace = m_surface.format.colorSpace;
	createInfo.imageExtent = m_surface.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = m_swapchain.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = m_surface.mode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// if the graphics and presentation aren't the same then use concurrent sharing mode
	uint32_t queueIndicies[] = { m_queue.graphIndex, m_queue.presentIndex };

	if (m_queue.graphIndex != m_queue.presentIndex)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueIndicies;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	// And finally, create the swap chain
	VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device.device, &createInfo, nullptr, &m_swapchain.swapChain));

	// Get the image loactions created when creating the swap chains
	vkGetSwapchainImagesKHR(m_device.device, m_swapchain.swapChain, &m_swapchain.imageCount, nullptr);
	m_swapchain.images.resize(m_swapchain.imageCount);
	vkGetSwapchainImagesKHR(m_device.device, m_swapchain.swapChain, &m_swapchain.imageCount, m_swapchain.images.data());
}

VkImageView VulkanCore::InitImageView(VkImage image, VkFormat format, VkImageAspectFlagBits imageAspect, VkImageViewType type)
{

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = imageAspect;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VkImageView imageView;
	VK_CHECK_RESULT(vkCreateImageView(m_device.device, &createInfo, nullptr, &imageView));

	return imageView;
}

void VulkanCore::Release()
{
	for (int c = 0; c < m_imageView.images.size(); ++c)
		vkDestroyImageView(m_device.device, m_imageView.images[c], nullptr);

	vkDestroySwapchainKHR(m_device.device, m_swapchain.swapChain, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface.surface, nullptr);

	vkDestroySemaphore(m_device.device, m_semaphore.render, nullptr);
	vkDestroySemaphore(m_device.device, m_semaphore.image, nullptr);
	vkDestroyDevice(m_device.device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}