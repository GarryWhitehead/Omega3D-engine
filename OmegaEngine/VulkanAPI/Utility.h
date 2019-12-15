#pragma once

#include "VulkanAPI/Common.h"

#include <vector>

namespace VulkanAPI
{

namespace Util
{

static vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu);

static vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu);
}
}    // namespace VulkanAPI