#include "VulkanInstance.h"
#include "VulkanCore/vulkan_validation.h"
#include "utility/file_log.h"

VulkanInstance::VulkanInstance() 
{
}


VulkanInstance::~VulkanInstance()
{
}

void VulkanInstance::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "OmegaOne";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	const char **glfwExtension;
	uint32_t count;
	std::vector<const char*> extensions;

	glfwExtension = glfwGetRequiredInstanceExtensions(&count);

	for (int c = 0; c < count; ++c)
		extensions.push_back(glfwExtension[c]);

#ifdef VULKAN_VALIDATION_DEBUG

	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);		// if validation layers are enabled, also add the debug ext to the GLFW extensions

	// check that the required layer validation is supported before continuing
	if (!ValidationLayers::FindValidationLayers())
	{
		*g_filelog << "Critical error! Unable to initialise required validation layers.";
		exit(EXIT_FAILURE);
	}
#endif

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = count;
	createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef VULKAN_VALIDATION_DEBUG
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = ValidationLayers::mReqLayers;
#else
		createInfo.enabledLayerCount = 0;
#endif

	VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
}

void VulkanInstance::PrepareWindowSurface(GLFWwindow *window)
{
	// begin by initialising the window surface
	VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}