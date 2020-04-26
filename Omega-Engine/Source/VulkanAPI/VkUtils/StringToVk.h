#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

namespace VkUtils
{

vk::CompareOp vkCompareOpFromString(std::string str);
vk::PolygonMode vkPolygonFromString(std::string str);
vk::FrontFace vkFrontFaceFromString(std::string str);
vk::CullModeFlagBits vkCullModeFromString(std::string str);
vk::Filter vkFilterToString(std::string str);
vk::SamplerAddressMode vkAddressModeToString(std::string str);
vk::DescriptorType getVkDescrTypeFromStr(std::string str);
vk::StencilOp vkStencilOpFromString(std::string str);

}    // namespace VkUtils
}    // namespace VulkanAPI
