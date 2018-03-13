#pragma once
#include "VulkanCore/vulkan_core.h"

//#ifdef NDEBUG
const bool EnableValidationLayers = true;
//#else 
//const bool EnableValidationLayers = false;
//#endif

class Device;

const int REQUIRED_VALID_LAYERS = 1;

class ValidationLayers
{
public:
	
	ValidationLayers();

	bool FindValidationLayers();
	void InitDebugCallBack(VkInstance instance);
	VkResult CreateDebugReportCallBackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* create_info_p, const VkAllocationCallbacks* alloc_p, VkDebugReportCallbackEXT* callback_p);
	void DestroyDebugReportCallBackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* alloc_p);
	void ReleaseValidation(VkInstance instance);

	friend class VulkanCore;

private:

	const char *mReqLayers[REQUIRED_VALID_LAYERS] = { "VK_LAYER_LUNARG_standard_validation" };
	VkDebugReportCallbackEXT m_debug_callback;

};