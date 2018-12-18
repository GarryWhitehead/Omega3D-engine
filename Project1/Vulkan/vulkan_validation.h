#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>


class ValidationLayers
{
public:
	
	static const int REQUIRED_VALID_LAYERS = 1;

	static constexpr char *mReqLayers[REQUIRED_VALID_LAYERS] = { "VK_LAYER_LUNARG_standard_validation" };

	ValidationLayers();

	static bool FindValidationLayers();
	static void InitDebugCallBack(VkInstance instance);
	static VkResult CreateDebugReportCallBackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* create_info_p, const VkAllocationCallbacks* alloc_p, VkDebugReportCallbackEXT* callback_p);
	static void DestroyDebugReportCallBackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* alloc_p);
	static void ReleaseValidation(VkInstance instance);

private:

	static VkDebugReportCallbackEXT m_debug_callback;

};