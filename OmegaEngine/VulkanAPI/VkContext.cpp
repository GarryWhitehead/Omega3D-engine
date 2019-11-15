#include "VkContext.h"

#include "utility/Logger.h"


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type,
                                                    uint64_t obj, size_t loc, int32_t code, const char* layer_prefix,
                                                    const char* msg, void* data)
{
	// ignore access mask false positive
	if (std::strcmp(layer_prefix, "DS") == 0 && code == 10)
	{
		return VK_FALSE;
	}

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		printf("Vulkan Error: %s: %\n", layer_prefix, msg);
		return VK_FALSE;
	}
	if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		printf("Vulkan Warning: %s: %\n", layer_prefix, msg);
		return VK_FALSE;
	}
	else
	{
		// just output this as a log
		LOGGER_INFO("Vulkan Information: %s: %s\n", layer_prefix, msg);
	}
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data)
{
	switch (severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		{
			printf("Validation Error: %s\n", data->pMessage);
		}
		else
		{
			printf("Other Error: %s\n", data->pMessage);
		}
		break;

	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		{
			printf("Validation Warning: %s\n", data->pMessage);
		}
		else
		{
			printf("Other Warning: %s\n", data->pMessage);
		}
		break;

	default:
		break;
	}

	bool logObjectNames = false;
	for (uint32_t i = 0; i < data->objectCount; i++)
	{
		auto* name = data->pObjects[i].pObjectName;
		if (name)
		{
			logObjectNames = true;
			break;
		}
	}

	if (logObjectNames)
	{
		for (uint32_t i = 0; i < data->objectCount; i++)
		{
			auto* name = data->pObjects[i].pObjectName;
			LOGGER_INFO("  Object #%u: %s\n", i, name ? name : "N/A");
		}
	}

	return VK_FALSE;
}

namespace VulkanAPI
{
static vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                                      vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu)
{
	vk::Format outputFormat;

	for (auto format : formats)
	{
		vk::FormatProperties properties = gpu.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && formatFeature == (properties.linearTilingFeatures & formatFeature))
		{
			outputFormat = format;
			break;
		}
		else if (tiling == vk::ImageTiling::eOptimal &&
		         formatFeature == (properties.optimalTilingFeatures & formatFeature))
		{
			outputFormat = format;
			break;
		}
		else
		{
			LOGGER_ERROR("Error! Unable to find supported vulkan format");
		}
	}
	return outputFormat;
}

bool VkContext::findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties)
{
	for (auto& ext : properties)
	{
		if (std::strcmp(ext.extensionName, name) == 0)
		{
			return true;
		}
	}
	return false;
};

vk::Format VkContext::getDepthFormat(vk::PhysicalDevice& gpu)
{
	// in order of preference - TODO: allow user to define whether stencil format is required or not
	std::vector<vk::Format> formats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint,
		                                vk::Format::eD32Sfloat };

	return VulkanUtil::findSupportedFormat(formats, vk::ImageTiling::eOptimal,
	                                       vk::FormatFeatureFlagBits::eDepthStencilAttachment, gpu);
}

void VkContext::createInstance(const char** glfwExtension, uint32_t extCount)
{
	vk::ApplicationInfo appInfo("OmegaEngine", VK_MAKE_VERSION(1, 1, 0), "", VK_MAKE_VERSION(1, 1, 0),
	                            VK_API_VERSION_1_1);

	// glfw extensions
	std::vector<const char*> extensions;
	for (uint32_t c = 0; c < extCount; ++c)
	{
		extensions.push_back(glfwExtension[c]);
	}

	// extension properties
	std::vector<vk::ExtensionProperties> extensionProps = vk::enumerateInstanceExtensionProperties();

	for (uint32_t i = 0; i < extCount; ++i)
	{
		if (!findExtensionProperties(extensions[i], extensionProps))
		{
			LOGGER_ERROR("Unable to find required extension properties for GLFW.");
		}
	}

	if (findExtensionProperties(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extensionProps))
	{
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		deviceExtensions.hasPhysicalDeviceProps2 = true;

		if (findExtensionProperties(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, extensionProps) &&
		    findExtensionProperties(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, extensionProps))
		{
			extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
			extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
			deviceExtensions.hasExternalCapabilities = true;
		}
	}
	if (findExtensionProperties(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProps))
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		deviceExtensions.hasDebugUtils = true;
	}

	// layer extensions, if any
	std::vector<vk::LayerProperties> layerExt = vk::enumerateInstanceLayerProperties();

	auto findLayerExtension = [&](const char* name) -> bool {
		for (auto& ext : layerExt)
		{
			if (std::strcmp(ext.layerName, name) == 0)
			{
				return true;
			}
		}
		return false;
	};

#ifdef VULKAN_VALIDATION_DEBUG

	if (findLayerExtension("VK_LAYER_LUNARG_standard_validation"))
	{
		requiredLayers.push_back("VK_LAYER_LUNARG_standard_validation");
	}
	else
	{
		LOGGER_INFO("Unable to find validation standard layers.");
	}

	// if debug utils isn't supported, try debug report
	if (!deviceExtensions.hasDebugUtils && findExtensionProperties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps))
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
#endif

	vk::InstanceCreateInfo createInfo({}, &appInfo, static_cast<uint32_t>(requiredLayers.size()), requiredLayers.data(),
	                                  static_cast<uint32_t>(extensions.size()), extensions.data());

	VK_CHECK_RESULT(vk::createInstance(&createInfo, nullptr, &instance));

#ifdef VULKAN_VALIDATION_DEBUG

	vk::DispatchLoaderDynamic dldi(instance);

	if (deviceExtensions.hasDebugUtils)
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo(
		    {},
		    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
		        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
		    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		    DebugMessenger, this);

		auto debugReport = instance.createDebugUtilsMessengerEXT(&createInfo, nullptr, &debugMessenger, dldi);
	}
	else if (findExtensionProperties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps))
	{
		vk::DebugReportCallbackCreateInfoEXT createInfo(vk::DebugReportFlagBitsEXT::eError |
		                                                    vk::DebugReportFlagBitsEXT::eWarning |
		                                                    vk::DebugReportFlagBitsEXT::ePerformanceWarning,
		                                                DebugCallback, this);

		instance.createDebugReportCallbackEXT(&createInfo, nullptr, &debugCallback, dldi);
	}
#endif
}

void VkContext::prepareDevice()
{
	if (!instance)
	{
		LOGGER_ERROR("You must first create a vulkan instnace before creating the device!");
	}

	// find a suitable gpu - at the moment this is pretty basic - find a gpu and that will do. In the future, find the best match
	std::vector<vk::PhysicalDevice> gpus = instance.enumeratePhysicalDevices();
	for (auto const& gpu : gpus)
	{
		if (gpu)
		{
			physical = gpu;
			break;
		}
	}

	if (!physical)
	{
		LOGGER_ERROR("Critcal error! No Vulkan supported GPU devices were found.");
	}

	// Also get all the device extensions for querying later
	std::vector<vk::ExtensionProperties> extensions = physical.enumerateDeviceExtensionProperties();

	// find queues for this gpu
	std::vector<vk::QueueFamilyProperties> queues = physical.getQueueFamilyProperties();

	// presentation queue
	for (uint32_t c = 0; c < queues.size(); ++c)
	{
		VkBool32 hasPresentionQueue = false;
		physical.getSurfaceSupportKHR(c, windowSurface, &hasPresentionQueue);
		if (queues[c].queueCount > 0 && hasPresentionQueue)
		{
			queueFamilyIndex.present = c;
			break;
		}
	}

	// graphics queue - if possible, use seperate queues for compute and graphic transfer
	for (uint32_t c = 0; c < queues.size(); ++c)
	{
		if (queues[c].queueCount > 0 && queues[c].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			queueFamilyIndex.graphics = c;
			break;
		}
	}

	// compute queue
	for (uint32_t c = 0; c < queues.size(); ++c)
	{
		if (queues[c].queueCount > 0 && c != queueFamilyIndex.present &&
		    queues[c].queueFlags & vk::QueueFlagBits::eCompute)
		{
			queueFamilyIndex.compute = c;
			break;
		}
	}

	// graphics and presentation queues are compulsory
	if (queueFamilyIndex.present == VK_QUEUE_FAMILY_IGNORED)
	{
		LOGGER_ERROR("Critcal error! Required queues not found.");
	}

	// The preference is a sepearte compute queue as this will be faster, though if not found, use the graphics queue for compute shaders
	if (queueFamilyIndex.compute == VK_QUEUE_FAMILY_IGNORED)
	{
		queueFamilyIndex.compute = queueFamilyIndex.graphics;
	}

	float queuePriority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> queueInfo = {};
	std::set<int> uniqueQueues = { queueFamilyIndex.graphics, queueFamilyIndex.present, queueFamilyIndex.compute };

	for (auto& queue : uniqueQueues)
	{
		vk::DeviceQueueCreateInfo createInfo({}, queue, 1, &queuePriority);
		queueInfo.push_back(createInfo);
	}

	// enable required device features
	vk::PhysicalDeviceFeatures requiredFeatures;
	vk::PhysicalDeviceFeatures features = physical.getFeatures();
	if (features.textureCompressionETC2)
	{
		requiredFeatures.textureCompressionETC2 = VK_TRUE;
	}
	if (features.textureCompressionBC)
	{
		requiredFeatures.textureCompressionBC = VK_TRUE;
	}
	if (features.samplerAnisotropy)
	{
		requiredFeatures.samplerAnisotropy = VK_TRUE;
	}
	if (features.tessellationShader)
	{
		requiredFeatures.tessellationShader = VK_TRUE;
	}
	if (features.geometryShader)
	{
		requiredFeatures.geometryShader = VK_TRUE;
	}
	if (features.shaderStorageImageExtendedFormats)
	{
		requiredFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
	}

	const std::vector<const char*> swapChainExtension = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	if (!findExtensionProperties(swapChainExtension[0], extensions))
	{
		LOGGER_ERROR("Critical error! Swap chain extension not found.");
	}

	vk::DeviceCreateInfo createInfo(
	    {}, static_cast<uint32_t>(queueInfo.size()), queueInfo.data(), static_cast<uint32_t>(requiredLayers.size()),
	    requiredLayers.empty() ? nullptr : requiredLayers.data(), static_cast<uint32_t>(swapChainExtension.size()),
	    swapChainExtension.data(), &requiredFeatures);

	VK_CHECK_RESULT(physical.createDevice(&createInfo, nullptr, &device));

	// prepare queue for each type
	vk::Queue vkComputeQueue, vkgraphicsQueue, vkPresentQueue;

	device.getQueue(queueFamilyIndex.compute, 0, &vkComputeQueue);
	device.getQueue(queueFamilyIndex.graphics, 0, &vkgraphicsQueue);
	device.getQueue(queueFamilyIndex.present, 0, &vkPresentQueue);

	// init vulkan api queue wrapper
	graphicsQueue.create(vkgraphicsQueue, device, queueFamilyIndex.graphics);
	presentQueue.create(vkPresentQueue, device, queueFamilyIndex.present);
	computeQueue.create(vkComputeQueue, device, queueFamilyIndex.compute);
}

uint32_t VkContext::getQueueIndex(QueueType type) const
{
	switch (type)
	{
	case QueueType::Graphics:
		return queueFamilyIndex.graphics;
		break;
	case QueueType::Present:
		return queueFamilyIndex.present;
		break;
	case QueueType::Compute:
		return queueFamilyIndex.compute;
		break;
	default:
		return -1;
	}
}

VulkanAPI::Queue VkContext::getQueue(QueueType type)
{
	VulkanAPI::Queue ret_queue;

	switch (type)
	{
	case QueueType::Graphics:
		ret_queue = graphicsQueue;
		break;
	case QueueType::Present:
		ret_queue = presentQueue;
		break;
	case QueueType::Compute:
		ret_queue = computeQueue;
		break;
	}

	return ret_queue;
}

}    // namespace VulkanAPI