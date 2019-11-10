#pragma once

#include <cstddef>
#include <string>

namespace VulkanAPI
{
namespace VkUtils
{

static bool createVkShaderInput(std::string name, std::string type, uint16_t bind, uint16_t setCount, std::string& output);

}
}    // namespace VulkanAPI
