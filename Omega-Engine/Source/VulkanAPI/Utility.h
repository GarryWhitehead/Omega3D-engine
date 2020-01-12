#pragma once

#include "VulkanAPI/Common.h"

#include "ImageUtils/KtxParser.h"

#include "ModelImporter/MeshInstance.h"

#include <vector>

namespace VulkanAPI
{

namespace VkUtil
{

vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu);

vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu);

vk::Format imageFormatToVk(const OmegaEngine::ImageFormat imageFormat);

vk::PrimitiveTopology topologyToVk(const OmegaEngine::Topology topology);
}
}    // namespace VulkanAPI
