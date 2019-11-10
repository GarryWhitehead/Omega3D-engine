#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

namespace VkUtils
{

static vk::CompareOp vkCompareOpFromString(std::string str);
static vk::PolygonMode vkPolygonFromString(std::string str);
static vk::FrontFace vkFrontFaceFromString(std::string str);
static vk::CullModeFlagBits vkCullModeFromString(std::string str);
static vk::Filter vkFilterToString(std::string str);
static vk::SamplerAddressMode vkAddressModeToString(std::string str);

}    // namespace VkUtils
}    // namespace VulkanAPI
