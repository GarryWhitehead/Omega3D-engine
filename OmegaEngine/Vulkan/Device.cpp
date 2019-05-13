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

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, 
														uint64_t obj, size_t loc, int32_t code, const char *layer_prefix, const char *msg, void *data)
	{
		// ignore access mask false positive
		if (strcmp(layer_prefix, "DS") == 0 && code == 10) 
		{
			return VK_FALSE;
		}

		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
		{
			std::cerr << "Vulkan Error:" << layer_prefix << ": " << msg << "\n";
			return VK_FALSE;
		}
		if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) 
		{
			std::cerr << "Vulkan Warning:" << layer_prefix << ": " << msg << "\n";
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
					std::cerr << "Validation Error: " << data->pMessage << "\n";
				}
				else
				{
					std::cerr << "Other Error: " << data->pMessage << "\n";
				}
				break;

			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				if (type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
				{
					std::cerr << "Validation Warning: " << data->pMessage << "\n";
				}
				else 
				{
					std::cerr << "Other Warning: " << data->pMessage << "\n";
				}
				break;

			default:
				break;
		}

		bool log_object_names = false;
		for (uint32_t i = 0; i < data->objectCount; i++) 
		{
			auto *name = data->pObjects[i].pObjectName;
			if (name) 
			{
				log_object_names = true;
				break;
			}
		}

		if (log_object_names) 
		{
			for (uint32_t i = 0; i < data->objectCount; i++) 
			{
				auto *name = data->pObjects[i].pObjectName;
				LOGGER_INFO("  Object #%u: %s\n", i, name ? name : "N/A");
			}
		}

		return VK_FALSE;
	}

	// static functions - should probably get elsewhere at some point
	namespace Util
	{
		static vk::Format find_supported_format(std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags format_feature, vk::PhysicalDevice& gpu)
		{
			vk::Format output_format;

			for (auto format : formats) 
			{
				vk::FormatProperties properties = gpu.getFormatProperties(format);

				if (tiling == vk::ImageTiling::eLinear && format_feature == (properties.linearTilingFeatures & format_feature)) 
				{
					output_format = format;
				}
				else if (tiling == vk::ImageTiling::eOptimal && format_feature == (properties.optimalTilingFeatures & format_feature)) 
				{
					output_format = format;
				}
				else 
				{
					LOGGER_ERROR("Error! Unable to find supported vulkan format");
				}
			}
			return output_format;
		}
	}

	bool Device::find_ext_properties(const char* name, std::vector<vk::ExtensionProperties>& properties)
	{
		for (auto& ext : properties) 
		{
			if (strcmp(ext.extensionName, name)) 
			{
				return true;
			}
		}
		return false;
	};

	vk::Format Device::get_depth_format(vk::PhysicalDevice& gpu)
	{
		// in order of preference - TODO: allow user to define whether stencil format is required or not
		std::vector<vk::Format> formats =
		{
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD32Sfloat
		};

		return Util::find_supported_format(formats, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment, gpu);
		
	}

	void Device::createInstance(const char **glfwExtension, uint32_t extCount)
	{
		vk::ApplicationInfo appInfo(
			"OmegaEngine",
			VK_MAKE_VERSION(1, 1, 0),
			"",
			VK_MAKE_VERSION(1, 1, 0),
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
			if (!find_ext_properties(extensions[i], extensionProps)) 
			{
				LOGGER_ERROR("Unable to find required extension properties for GLFW.");
				throw std::runtime_error("Unable to initiliase Vulkan due to missing extension properties.");
			}
		}

		if (find_ext_properties(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extensionProps)) 
		{
			extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			device_ext.has_physical_device_props2 = true;

			if (find_ext_properties(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, extensionProps) && find_ext_properties(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, extensionProps)) 
			{
				extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
				extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
				device_ext.has_external_capabilities = true;
			}
		}
		if (find_ext_properties(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extensionProps)) 
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			device_ext.has_debug_utils = true;
		}

		// layer extensions, if any
		std::vector<vk::LayerProperties> layerExt = vk::enumerateInstanceLayerProperties();

		auto findLayerExt = [&](const char *name) -> bool 
		{
			for (auto& ext : layerExt) 
			{
				if (strcmp(ext.layerName, name)) 
				{
					return true;
				}
			}
			return false;
		};

#ifdef VULKAN_VALIDATION_DEBUG

		if (findLayerExt("VK_LAYER_LUNARG_standard_validation")) 
		{
			req_layers.push_back("VK_LAYER_LUNARG_standard_validation");
		}
		else 
		{
			LOGGER_INFO("Unable to find validation standard layers.");
		}

		// if debug utils isn't supported, try debug report
		if (!device_ext.has_debug_utils && find_ext_properties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps)) 
		{
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);		
		}
#endif

		vk::InstanceCreateInfo createInfo({},
		&appInfo,
		static_cast<uint32_t>(req_layers.size()),
		req_layers.data(),
		static_cast<uint32_t>(extensions.size()),
		extensions.data());

		VK_CHECK_RESULT(vk::createInstance(&createInfo, nullptr, &instance));

#ifdef VULKAN_VALIDATION_DEBUG

		vk::DispatchLoaderDynamic dldi(instance);

		if (device_ext.has_debug_utils) 
		{
			vk::DebugUtilsMessengerCreateInfoEXT create_info({},
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
				DebugMessenger, this);

			auto debugReport = instance.createDebugUtilsMessengerEXT(&create_info, nullptr, &debug_messenger, dldi);
		}
		else if (find_ext_properties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps)) 
		{
			vk::DebugReportCallbackCreateInfoEXT create_info(
				vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning,
				DebugCallBack, this);

			instance.createDebugReportCallbackEXT(&create_info, nullptr, &debug_callback, dldi);
		}
#endif
	}

	void Device::prepareDevice()
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
			VkBool32 have_present_queue = false;
			physical.getSurfaceSupportKHR(c, win_surface, &have_present_queue);
			if (queues[c].queueCount > 0 && have_present_queue) 
			{
				queue_family_index.present = c;
				break;
			}
		}

		// graphics queue - if possible, use seperate queues for compute and graphic transfer
		for (uint32_t c = 0; c < queues.size(); ++c)
		{
			if (queues[c].queueCount > 0 && queues[c].queueFlags & vk::QueueFlagBits::eGraphics) 
			{
				queue_family_index.graphics = c;
				break;
			}
		}

		// compute queue
		for (uint32_t c = 0; c < queues.size(); ++c)
		{
			if (queues[c].queueCount > 0 && c != queue_family_index.present && queues[c].queueFlags & vk::QueueFlagBits::eCompute) 
			{
				queue_family_index.compute = c;
				break;
			}
		}

		// graphics and presentation queues are compulsory
		if (queue_family_index.present == VK_QUEUE_FAMILY_IGNORED)
		{
			LOGGER_ERROR("Critcal error! Required queues not found.");
		}

		// The preference is a sepearte compute queue as this will be faster, though if not found, use the graphics queue for compute shaders
		if (queue_family_index.compute == VK_QUEUE_FAMILY_IGNORED) 
		{
			queue_family_index.compute = queue_family_index.graphics;
		}

		float queuePriority = 1.0f;
		std::vector<vk::DeviceQueueCreateInfo> queueInfo = {};
		std::set<int> uniqueQueues = { queue_family_index.graphics, queue_family_index.present, queue_family_index.compute };

		for (auto& queue : uniqueQueues) 
		{
			vk::DeviceQueueCreateInfo createInfo({}, queue, 1, &queuePriority);
			queueInfo.push_back(createInfo);
		}

		// enable required device features
		vk::PhysicalDeviceFeatures req_features;
		vk::PhysicalDeviceFeatures features = physical.getFeatures();
		if (features.textureCompressionETC2) 
		{
			req_features.textureCompressionETC2 = VK_TRUE;
		}
		if (features.textureCompressionBC) 
		{
			req_features.textureCompressionBC = VK_TRUE;
		}
		if (features.samplerAnisotropy)
		{
			req_features.samplerAnisotropy = VK_TRUE;
		}
		if (features.tessellationShader) 
		{
			req_features.tessellationShader = VK_TRUE;
		}
		if (features.geometryShader) 
		{
			req_features.geometryShader = VK_TRUE;
		}
		if (features.shaderStorageImageExtendedFormats) 
		{
			req_features.shaderStorageImageExtendedFormats = VK_TRUE;
		}

		const std::vector<const char*> swapChainExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		if (!find_ext_properties(swapChainExt[0], extensions)) 
		{
			LOGGER_ERROR("Critical error! Swap chain extension not found.");
			throw std::runtime_error("No swap chain extensions found.");
		}

		vk::DeviceCreateInfo createInfo({}, 
		static_cast<uint32_t>(queueInfo.size()), queueInfo.data(),
		static_cast<uint32_t>(req_layers.size()), req_layers.empty() ? nullptr : req_layers.data(),
		static_cast<uint32_t>(swapChainExt.size()), swapChainExt.data(),
		&req_features);

		VK_CHECK_RESULT(physical.createDevice(&createInfo, nullptr, &device));

		// prepare queue for each type
		vk::Queue vkComputeQueue, vkGraphQueue, vkPresentQueue;

		device.getQueue(queue_family_index.compute, 0, &vkComputeQueue);
		device.getQueue(queue_family_index.graphics, 0, &vkGraphQueue);
		device.getQueue(queue_family_index.present, 0, &vkPresentQueue);

		// init vulkan api queue wrapper
		graphQueue.create(vkGraphQueue, device, queue_family_index.graphics);
		presentQueue.create(vkPresentQueue, device, queue_family_index.present);
		computeQueue.create(vkComputeQueue, device, queue_family_index.compute);
	}

	uint32_t Device::getQueueIndex(QueueType type) const
	{
		switch (type) 
		{
			case QueueType::Graphics:
				return queue_family_index.graphics;
				break;
			case QueueType::Present:
				return queue_family_index.present;
				break;
			case QueueType::Compute:
				return queue_family_index.compute;
				break;
			default:
				return -1;
		}
	}

	VulkanAPI::Queue Device::getQueue(QueueType type)
	{
		VulkanAPI::Queue ret_queue;

		switch (type) 
		{
			case QueueType::Graphics:
				ret_queue = graphQueue;
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
}
