#include "Device.h"

#include "Utility/logger.h"
#include <assert.h>
#include <iostream>

namespace VulkanAPI
{

	Device::Device()
	{
	}


	Device::~Device()
	{
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(vk::DebugReportFlagsEXT flags, vk::DebugReportObjectTypeEXT obj_type, uint64_t obj, size_t loc, int32_t code, const char *layer_prefix, const char *msg, void *data)
	{
		std::cerr << "Validation Layer: " << msg << "\n";
		return VK_FALSE;
	}

	// static functions - should probably get elsewhere at some point
	namespace Util
	{
		static vk::Format& find_supported_format(std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags format_feature, vk::PhysicalDevice& gpu)
		{
			for (auto format : formats) {
				vk::FormatProperties properties = gpu.getFormatProperties(format);

				if (tiling == vk::ImageTiling::eLinear && format_feature == (properties.linearTilingFeatures & format_feature)) {
					return format;
				}
				else if (tiling == vk::ImageTiling::eOptimal && format_feature == (properties.optimalTilingFeatures & format_feature)) {
					return format;
				}
				else {
					throw std::runtime_error("Error! Unable to find supported vulkan format");
				}
			}
		}

		static vk::Format& get_depth_format(vk::PhysicalDevice& gpu)
		{
			std::vector<vk::Format> formats =
			{
				vk::Format::eD32Sfloat,
				vk::Format::eD32SfloatS8Uint,
				vk::Format::eD24UnormS8Uint
			};

			vk::Format depthFormat = find_supported_format(formats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment, gpu);
		}
	}

	void Device::createInstance(const char **glfwExtension, uint32_t extCount)
	{
		vk::ApplicationInfo appInfo(
			"OmegaOne",
			VK_MAKE_VERSION(1, 1, 0),
			"",
			VK_MAKE_VERSION(1, 1, 0),
			VK_API_VERSION_1_1);

		// glfw extensions
		std::vector<const char*> extensions;
		for (uint32_t c = 0; c < extCount; ++c) {
			extensions.push_back(glfwExtension[c]);
		}

		// extension properties
		std::vector<vk::ExtensionProperties> extensionProps = vk::enumerateInstanceExtensionProperties();

		auto findExtProp = [&](const char *name) -> bool {
			for (auto& ext : extensionProps) {
				if (strcmp(ext.extensionName, name)) {
					return true;
				}
			}
			return false;
		};

		for (uint32_t i = 0; i < extCount; ++i) {
			if (!findExtProp(extensions[i])) {
				LOGGER_ERROR("Unable to find required extension properties for GLFW.");
				throw std::runtime_error("Unable to initiliase Vulkan due to missing extension properties.");
			}
		}

		if (findExtProp(VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME)) {
			extensions.push_back(VK_KHR_GET_DISPLAY_PROPERTIES_2_EXTENSION_NAME);
			device_ext.has_physical_device_props2 = true;

			if (findExtProp(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) && findExtProp(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME)) {
				extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
				extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
				device_ext.has_external_capabilities = true;
			}
		}
		if (findExtProp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			device_ext.has_debug_utils = true;
		}

		// layer extensions, if any
		std::vector<vk::LayerProperties> layerExt = vk::enumerateInstanceLayerProperties();

		auto findLayerExt = [&](const char *name) -> bool {
			for (auto& ext : layerExt) {
				if (strcmp(ext.layerName, name)) {
					return true;
				}
			}
			return false;
		};

#ifdef VULKAN_VALIDATION_DEBUG

		if (findLayerExt("VK_LAYER_LUNARG_standard_validation")) {
			layer_ext.push_back("VK_LAYER_LUNARG_standard_validation");
		}
		else {
			LOGGER_INFO("Unable to find validation standard layers.");
		}

		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);		// if validation layers are enabled, also add the debug ext to the GLFW extensions
#endif

		vk::InstanceCreateInfo createInfo({},
		&appInfo,
		static_cast<uint32_t>(layers.size()),
		layers.data(),
		static_cast<uint32_t>(extensions.size()),
		extensions.data());

		VK_CHECK_RESULT(vk::createInstance(&createInfo, nullptr, &instance));

		if (device_ext.has_debug_utils) {

			vk::DebugUtilsMessengerCreateInfoEXT({},
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
				vk_debug_callback,
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
				this
			);
		}
	}

	void Device::prepareDevice()
	{
		if (!instance) {
			LOGGER_ERROR("You must first create a vulkan instnace before creating the device!");
			throw std::runtime_error("Error whilst initialising device");
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
			throw std::runtime_error("Unable to find any vulkan-supported GPUs");
		}

		// Also get all the device extensions for querying later
		std::vector<vk::ExtensionProperties> extensions = physical.enumerateDeviceExtensionProperties();

		// find queues for this gpu
		uint32_t queueCount = 0;
		VkBool32 have_present_queue = false;

		std::vector<vk::QueueFamilyProperties> queues = physical.getQueueFamilyProperties();

		for (uint32_t c = 0; c < queues.size(); ++c)
		{
			if (queues[c].queueCount > 0 && queues[c].queueFlags & vk::QueueFlagBits::eGraphics) {									// graphics queue?
				queue.graphIndex = c;
			}

			if (queues[c].queueCount > 0 && queues[c].queueFlags & vk::QueueFlagBits::eCompute) {									// compute queue?
				queue.computeIndex = c;
			}

			physical.getSurfaceSupportKHR(c, surface, &have_present_queue);
			if (queues[c].queueCount > 0 && have_present_queue) {																	// presentation queue?
				queue.presentIndex = c;
			}

			if (queue.graphIndex > 0 && queue.presentIndex > 0) {
				break;
			}
		}

		// graphics and presentation queues are compulsory
		if (queue.graphIndex == VK_QUEUE_FAMILY_IGNORED || queue.presentIndex == VK_QUEUE_FAMILY_IGNORED)
		{
			LOGGER_ERROR("Critcal error! Required queues not found.");
			throw std::runtime_error("Error whilst initialising gfx and present queues.");
		}

		// The preference is a sepearte compute queue as this will be faster, though if not found, use the graphics queue for compute shaders
		else if (queue.computeIndex == VK_QUEUE_FAMILY_IGNORED) {
			
			queue.computeIndex = queue.graphIndex;
		}

		float queuePriority = 1.0f;

		std::vector<vk::DeviceQueueCreateInfo> queueInfo = {};
		std::set<int> uniqueQueues = { queue.graphIndex, queue.presentIndex, queue.computeIndex };

		for (auto& queue : uniqueQueues) {
			vk::DeviceQueueCreateInfo createInfo({}, queue, 1, &queuePriority);
			queueInfo.push_back(createInfo);
		}

		// enable required device features
		vk::PhysicalDeviceFeatures req_features;
		vk::PhysicalDeviceFeatures features = physical.getFeatures();
		if (features.textureCompressionETC2) {
			req_features.textureCompressionETC2 = VK_TRUE;
		}
		if (features.textureCompressionBC) {
			req_features.textureCompressionBC = VK_TRUE;
		}
		if (features.samplerAnisotropy) {
			req_features.samplerAnisotropy = VK_TRUE;
		}
		if (features.tessellationShader) {
			req_features.tessellationShader = VK_TRUE;
		}
		if (features.geometryShader) {
			req_features.geometryShader = VK_TRUE;
		}
		if (features.shaderStorageImageExtendedFormats) {
			req_features.shaderStorageImageExtendedFormats = VK_TRUE;
		}

		const std::vector<const char*> swapChainExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		if (!FindDeviceExtenisions(swapChainExt[0])) {
			LOGGER_ERROR("Critical error! Swap chain extension not found.");
			throw std::runtime_error("No swap chain extensions found.");
		}

		vk::DeviceCreateInfo createInfo({}, 
		static_cast<uint32_t>(queueInfo.size()),
		queueInfo.data(),
		layers.size(),
		layers.data(),
		static_cast<uint32_t>(swapChainExt.size()),
		swapChainExt.data(),
		&req_features
		);

#ifdef VULKAN_VALIDATION_DEBUG
		createInfo.enabledLayerCount = 1;
		createInfo.ppEnabledLayerNames = ValidationLayers::mReqLayers;
#else
		createInfo.enabledLayerCount = 0;
#endif

		VK_CHECK_RESULT(physical.createDevice(&createInfo, nullptr, &device));

		// this reduces dispatch overhead - though instigates that only one device can be used
		//volkLoadDevice(device.operator VkDevice);

		// prepare queue for each type
		vk::Queue computeQueue, graphQueue, presentQueue;

		device.getQueue(queue.computeIndex, 0, &computeQueue);
		device.getQueue(queue.graphIndex, 0, &graphQueue);
		device.getQueue(queue.presentIndex, 0, &presentQueue);

		// init vulkan api queue wrapper
		queue.graphQueue.create(graphQueue, device);
		queue.presentQueue.create(presentQueue, device);
		queue.computeQueue.create(computeQueue, device);
	}

	uint32_t Device::getQueueIndex(QueueType type) const
	{
		switch (type) {
		case QueueType::Graphics:
			return queue.graphIndex;
			break;
		case QueueType::Present:
			return queue.presentIndex;
			break;
		case QueueType::Compute:
			return queue.computeIndex;
			break;
		default:
			return -1;
		}
	}

	VulkanAPI::Queue& Device::getQueue(QueueType type)
	{
		VulkanAPI::Queue ret_queue;

		switch (type) {
		case QueueType::Graphics:
			ret_queue = queue.graphQueue;
			break;
		case QueueType::Present:
			ret_queue = queue.presentQueue;
			break;
		case QueueType::Compute:
			ret_queue = queue.computeQueue;
			break;
		}

		return ret_queue;
	}
}
