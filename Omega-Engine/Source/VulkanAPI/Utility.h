#pragma once

#include "ImageUtils/KtxParser.h"
#include "ModelImporter/MeshInstance.h"
#include "VulkanAPI/Common.h"

#include <vector>

namespace VulkanAPI
{

namespace VkUtil
{

bool isDepth(const vk::Format format);

bool isStencil(const vk::Format format);

vk::Format findSupportedFormat(
    std::vector<vk::Format>& formats,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags formatFeature,
    vk::PhysicalDevice& gpu);

vk::Format getSupportedDepthFormat(vk::PhysicalDevice& gpu);

vk::Format imageFormatToVk(const OmegaEngine::ImageFormat imageFormat);

vk::PrimitiveTopology topologyToVk(const OmegaEngine::Topology topology);

// Takes a string of certain type and if valid, returns as a vulkan recognisible type.
vk::Format getVkFormatFromType(std::string type, uint32_t width);

// Derieves from the type specified, the stride in bytes
uint32_t getStrideFromType(std::string type);

} // namespace VkUtil
} // namespace VulkanAPI
