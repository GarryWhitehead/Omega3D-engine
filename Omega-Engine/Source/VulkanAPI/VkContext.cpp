#include "VkContext.h"

#include "VulkanAPI/CBufferManager.h"
#include "utility/Logger.h"

#include <set>
#include <string.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t obj,
    size_t loc,
    int32_t code,
    const char* layer_prefix,
    const char* msg,
    void* data)
{
    // ignore access mask false positive
    if (std::strcmp(layer_prefix, "DS") == 0 && code == 10)
    {
        return VK_FALSE;
    }

    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        printf("Vulkan Error: %s: %s\n", layer_prefix, msg);
        return VK_FALSE;
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        printf("Vulkan Warning: %s: %s\n", layer_prefix, msg);
        return VK_FALSE;
    }
    else
    {
        // just output this as a log
        LOGGER_INFO("Vulkan Information: %s: %s\n", layer_prefix, msg);
    }
    return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessenger(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
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

bool VkContext::findExtensionProperties(
    const char* name, std::vector<vk::ExtensionProperties>& properties)
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

bool VkContext::prepareExtensions(
    std::vector<const char*>& extensions,
    uint32_t extCount,
    std::vector<vk::ExtensionProperties>& extensionProps)
{
    for (uint32_t i = 0; i < extCount; ++i)
    {
        if (!findExtensionProperties(extensions[i], extensionProps))
        {
            LOGGER_ERROR("Unable to find required extension properties for GLFW.");
            return false;
        }
    }

    if (findExtensionProperties(
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, extensionProps))
    {
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        deviceExtensions.hasPhysicalDeviceProps2 = true;

        if (findExtensionProperties(
                VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, extensionProps) &&
            findExtensionProperties(
                VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, extensionProps))
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

#ifdef VULKAN_VALIDATION_DEBUG
    // if debug utils isn't supported, try debug report
    if (!deviceExtensions.hasDebugUtils &&
        findExtensionProperties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps))
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }
#endif
    return true;
}

bool VkContext::createInstance(const char** glfwExtension, uint32_t extCount)
{
    vk::ApplicationInfo appInfo(
        "OmegaEngine", VK_MAKE_VERSION(1, 1, 0), "", VK_MAKE_VERSION(1, 1, 0), VK_API_VERSION_1_1);

    // glfw extensions
    std::vector<const char*> extensions;
    for (uint32_t c = 0; c < extCount; ++c)
    {
        extensions.push_back(glfwExtension[c]);
    }

    // extension properties
    std::vector<vk::ExtensionProperties> extensionProps =
        vk::enumerateInstanceExtensionProperties();

    if (!prepareExtensions(extensions, extCount, extensionProps))
    {
        return false;
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
        LOGGER_WARN("Unable to find validation standard layers.");
    }
#endif

    vk::InstanceCreateInfo createInfo(
        {},
        &appInfo,
        static_cast<uint32_t>(requiredLayers.size()),
        requiredLayers.data(),
        static_cast<uint32_t>(extensions.size()),
        extensions.data());

    VK_CHECK_RESULT(vk::createInstance(&createInfo, nullptr, &instance));

#ifdef VULKAN_VALIDATION_DEBUG
    vk::DispatchLoaderDynamic dldi(instance);

    if (deviceExtensions.hasDebugUtils)
    {
        vk::DebugUtilsMessengerCreateInfoEXT dbgCreateInfo(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
            DebugMessenger,
            this);

        auto debugReport =
            instance.createDebugUtilsMessengerEXT(&dbgCreateInfo, nullptr, &debugMessenger, dldi);
    }
    else if (findExtensionProperties(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extensionProps))
    {
        vk::DebugReportCallbackCreateInfoEXT cbCreateInfo(
            vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |
                vk::DebugReportFlagBitsEXT::ePerformanceWarning,
            DebugCallback,
            this);

        instance.createDebugReportCallbackEXT(&cbCreateInfo, nullptr, &debugCallback, dldi);
    }
#endif

    return true;
}

vk::PhysicalDeviceFeatures VkContext::prepareFeatures()
{
    vk::PhysicalDeviceFeatures requiredFeatures;
    vk::PhysicalDeviceFeatures devFeatures = physical.getFeatures();
    if (devFeatures.textureCompressionETC2)
    {
        requiredFeatures.textureCompressionETC2 = VK_TRUE;
    }
    if (devFeatures.textureCompressionBC)
    {
        requiredFeatures.textureCompressionBC = VK_TRUE;
    }
    if (devFeatures.samplerAnisotropy)
    {
        requiredFeatures.samplerAnisotropy = VK_TRUE;
    }
    if (devFeatures.tessellationShader)
    {
        requiredFeatures.tessellationShader = VK_TRUE;
    }
    if (devFeatures.geometryShader)
    {
        requiredFeatures.geometryShader = VK_TRUE;
    }
    if (devFeatures.shaderStorageImageExtendedFormats)
    {
        requiredFeatures.shaderStorageImageExtendedFormats = VK_TRUE;
    }
    return requiredFeatures;
}

bool VkContext::prepareDevice(const vk::SurfaceKHR windowSurface)
{
    if (!instance)
    {
        LOGGER_ERROR("You must first create a vulkan instance before creating the device!");
        return false;
    }

    // find a suitable gpu - at the moment this is pretty basic - find a gpu and that will do. In
    // the future, find the best match
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
        return false;
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
        return false;
    }

    // The preference is a sepearte compute queue as this will be faster, though if not found, use
    // the graphics queue for compute shaders
    if (queueFamilyIndex.compute == VK_QUEUE_FAMILY_IGNORED)
    {
        queueFamilyIndex.compute = queueFamilyIndex.graphics;
    }

    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueInfo = {};
    std::set<uint32_t> uniqueQueues = {
        queueFamilyIndex.graphics, queueFamilyIndex.present, queueFamilyIndex.compute};

    for (auto& queue : uniqueQueues)
    {
        vk::DeviceQueueCreateInfo createInfo({}, queue, 1, &queuePriority);
        queueInfo.push_back(createInfo);
    }

    // enable required device features
    auto reqFeatures = prepareFeatures();

    const std::vector<const char*> swapChainExtension = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    if (!findExtensionProperties(swapChainExtension[0], extensions))
    {
        LOGGER_ERROR("Critical error! Swap chain extension not found.");
        return false;
    }

    vk::DeviceCreateInfo createInfo(
        {},
        static_cast<uint32_t>(queueInfo.size()),
        queueInfo.data(),
        static_cast<uint32_t>(requiredLayers.size()),
        requiredLayers.empty() ? nullptr : requiredLayers.data(),
        static_cast<uint32_t>(swapChainExtension.size()),
        swapChainExtension.data(),
        &reqFeatures);

    VK_CHECK_RESULT(physical.createDevice(&createInfo, nullptr, &device));

    // ================================= queues =============================================
    // prepare queue for each type
    device.getQueue(queueFamilyIndex.compute, 0, &computeQueue);
    device.getQueue(queueFamilyIndex.graphics, 0, &graphicsQueue);
    device.getQueue(queueFamilyIndex.present, 0, &presentQueue);

    return true;
}

} // namespace VulkanAPI
