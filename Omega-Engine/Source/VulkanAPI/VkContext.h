/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

/**
 * The current state of this vulkan instance. Encapsulates all information extracted from the device
 * and physical device. Also handles the state command buffer state
 *
 */
struct VkContext
{
    struct Extensions
    {
        bool hasPhysicalDeviceProps2 = false;
        bool hasExternalCapabilities = false;
        bool hasDebugUtils = false;
    };

    enum class QueueType
    {
        Graphics,
        Present,
        Compute,
        Count
    };

    VkContext() = default;

    static bool
    findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);

    bool prepareExtensions(
        std::vector<const char*>& extensions,
        uint32_t extCount,
        std::vector<vk::ExtensionProperties>& extensionProps);

    vk::PhysicalDeviceFeatures prepareFeatures();

    /**
     * @brief Creates a new abstract instance of vulkan
     */
    bool createInstance(const char** glfwExtension, uint32_t extCount);

    /**
     * @brief Sets up all the vulkan devices and queues.
     */
    bool prepareDevice(const vk::SurfaceKHR windowSurface);

public:
    
    vk::Instance instance;
    vk::Device device;
    vk::PhysicalDevice physical;
    vk::PhysicalDeviceFeatures features;

    struct QueueInfo
    {
        uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
        uint32_t present = VK_QUEUE_FAMILY_IGNORED;
        uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
    } queueFamilyIndex;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::Queue computeQueue;

    // supported extensions
    Extensions deviceExtensions;

private:

    // validation layers
    std::vector<const char*> requiredLayers;

#ifdef VULKAN_VALIDATION_DEBUG

    vk::DebugReportCallbackEXT debugCallback;
    vk::DebugUtilsMessengerEXT debugMessenger;

#endif
    
};


} // namespace VulkanAPI
