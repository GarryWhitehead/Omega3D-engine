#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace VulkanAPI
{
namespace VkUtils
{

static bool isSamplerType(const std::string& type);

static void createVkShaderInput(const std::string name, const std::string type, const uint16_t bind, const uint16_t setCount, std::string& output);

static void createVkShaderBuffer(const std::string name, const std::string type, const std::vector<std::pair<std::string, std::string>>& items, const uint16_t bind, const uint16_t setCount, std::string& output, uint32_t& bufferSize);
}
}    // namespace VulkanAPI
