#include "VkToString.h"

#include <cassert>

namespace VulkanAPI
{
namespace VkUtils
{

bool createVkShaderInput(std::string name, std::string type, uint16_t bind, uint16_t setCount, std::string& output)
{
	assert(!type.empty());
	assert(!name.empty());

	if (type == "sampler2D" || type == "sampler3D" || type == "samplerCube")
	{
		output = "layout (set = " + std::to_string(setCount) + ", binding = " + std::to_string(bind) +
		         ") uniform smapler2D " + name;
	}
	else if (type == "UniformBuffer")
	{
		output =
		    "layout (set = " + std::to_string(setCount) + ", binding = " + std::to_string(bind) + ") uniform " + name;
	}
}

}    // namespace VkUtils
}    // namespace VulkanAPI