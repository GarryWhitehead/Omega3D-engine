#include "VulkanCore/Vulkan_validation.h"
#include "utility/file_log.h"
#include "VulkanCore/vulkan_tools.h"
#include <iostream>



ValidationLayers::ValidationLayers() 
{
}



bool ValidationLayers::FindValidationLayers()
{
	uint32_t layers = 0;
	vkEnumerateInstanceLayerProperties(&layers, nullptr);
	if (layers == 0)
	{
		g_filelog->WriteLog("Critical error! No vaildation layers found.");
		exit(EXIT_FAILURE);
	}

	std::vector<VkLayerProperties> validProps(layers);
	vkEnumerateInstanceLayerProperties(&layers, validProps.data());

	bool success = false;
	for (const auto& available : validProps)
	{
		for (int c = 0; c < REQUIRED_VALID_LAYERS; ++c)
		{
			if (strcmp(available.layerName, mReqLayers[c]) == 0)
				success = true;
		}
	}
	return success;
}

void ValidationLayers::InitDebugCallBack(VkInstance instance)
{
	VkDebugReportCallbackCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	create_info.pfnCallback = DebugCallBack;

	VK_CHECK_RESULT(CreateDebugReportCallBackEXT(instance, &create_info, nullptr, &m_debug_callback));
}

VkResult ValidationLayers::CreateDebugReportCallBackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* create_info_p, const VkAllocationCallbacks* alloc_p, VkDebugReportCallbackEXT* callback_p)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
		return func(instance, create_info_p, alloc_p, callback_p);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void ValidationLayers::DestroyDebugReportCallBackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* alloc_p)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
		return func(instance, callback, alloc_p);
}

void ValidationLayers::ReleaseValidation(VkInstance instance)
{
	DestroyDebugReportCallBackEXT(instance, m_debug_callback, nullptr);
}
