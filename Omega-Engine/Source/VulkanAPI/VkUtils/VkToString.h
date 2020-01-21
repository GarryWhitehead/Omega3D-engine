#pragma once

#include "VulkanAPI/Compiler/ShaderParser.h"

#include <cstddef>
#include <string>
#include <vector>

namespace VulkanAPI
{
namespace VkUtils
{

bool isSamplerType(const std::string& type);

bool isBufferType(const std::string& type);

bool createVkShaderSampler(
    const std::string name,
    const std::string type,
    const uint16_t bind,
    const uint16_t setCount,
    std::string& output);

bool createVkShaderBuffer(
    const std::string name,
    const std::string type,
    const std::vector<ShaderDescriptor::TypeDescriptors>& items,
    const uint16_t bind,
    const uint16_t setCount,
    std::string& output,
    uint32_t& bufferSize);

} // namespace VkUtils
} // namespace VulkanAPI
