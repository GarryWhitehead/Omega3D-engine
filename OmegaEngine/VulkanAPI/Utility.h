#pragma once

#include "VulkanAPI/Common.h"

#include <vector>

namespace VulkanAPI
{

namespace VkUtil
{

vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu);

vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu);
}
}    // namespace VulkanAPI
