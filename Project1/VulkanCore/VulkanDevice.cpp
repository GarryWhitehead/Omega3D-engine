#include "VulkanDevice.h"
#include "utility/file_log.h"
#include "VulkanCore/vulkan_validation.h"
#include <set>

VulkanDevice::VulkanDevice()
{
}

VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::PrepareQueues(VkSurfaceKHR surface)
{
	uint32_t queueCount = 0;
	VkBool32 presentQueue = false;

	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
	std::vector<VkQueueFamilyProperties> queues(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, queues.data());

	for (uint32_t c = 0; c < queues.size(); ++c)
	{
		if (queues[c].queueCount > 0 && queues[c].queueFlags & VK_QUEUE_GRAPHICS_BIT) {									// graphics queue?
			queue.graphIndex = c;
		}

		if (queues[c].queueCount > 0 && queues[c].queueFlags & VK_QUEUE_COMPUTE_BIT) {									// compute queue?
			queue.computeIndex = c;
		}

		vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, c, surface, &presentQueue);
		if (queues[c].queueCount > 0 && presentQueue) {																	// presentation queue?
			queue.presentIndex = c;
		}

		if (queue.graphIndex > 0 && queue.presentIndex > 0) {
			break;
		}
	}

	if (queue.graphIndex < 0 || queue.presentIndex < 0)
	{
		g_filelog->WriteLog("Critcal error! Required queues not found.");
		exit(EXIT_FAILURE);
	}
	else if (queue.computeIndex < 0) {						// The preference is a sepearte compute queue, though if not found, use the graphics queue for compute shaders
		queue.computeIndex = queue.graphIndex;
	}
}

void VulkanDevice::PreparePhysicalDevice(const VkInstance instance)
{
	uint32_t count;
	vkEnumeratePhysicalDevices(instance, &count, nullptr);
	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(instance, &count, devices.data());

	for (auto const& dev : devices)
	{
		if (dev)
		{
			physDevice = dev;
			break;
		}
	}

	if (physDevice == VK_NULL_HANDLE)
	{
		g_filelog->WriteLog("Critcal error! No Vulkan supported GPU devices were found.");
		exit(EXIT_FAILURE);
	}
}

void VulkanDevice::PrepareDevice()
{
	float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueInfo = {};
	std::set<int> uniqueQueues = { queue.graphIndex, queue.presentIndex, queue.computeIndex };

	for (auto& queue : uniqueQueues) {
		VkDeviceQueueCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.queueFamilyIndex = queue;
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = &queuePriority;
		queueInfo.push_back(createInfo);
	}

	// enable required device features
	VkPhysicalDeviceFeatures devFeatures = {};
	devFeatures.samplerAnisotropy = VK_TRUE;
	devFeatures.tessellationShader = VK_TRUE;
	devFeatures.textureCompressionBC = VK_TRUE;
	devFeatures.geometryShader = VK_TRUE;
	devFeatures.shaderStorageImageExtendedFormats = VK_TRUE;

	const std::vector<const char*> swapChainExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	if (!FindDeviceExtenisions(physDevice, swapChainExt[0])) {
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

#ifdef VULKAN_VALIDATION_DEBUG
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = ValidationLayers::mReqLayers;
#else
		createInfo.enabledLayerCount = 0;
#endif

	VK_CHECK_RESULT(vkCreateDevice(physDevice, &createInfo, nullptr, &device));

	// Get the feature assocated with the physical device
	vkGetPhysicalDeviceFeatures(physDevice, &features);

	// prepare queue for each type
	vkGetDeviceQueue(device, queue.computeIndex, 0, &queue.computeQueue);
	vkGetDeviceQueue(device, queue.graphIndex, 0, &queue.graphQueue);
	vkGetDeviceQueue(device, queue.presentIndex, 0, &queue.presentQueue);
}

bool VulkanDevice::FindDeviceExtenisions(VkPhysicalDevice physDevice, const char* reqDevice)
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

void VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface)
{
	PreparePhysicalDevice(instance);
	PrepareQueues(surface);
	PrepareDevice();
}