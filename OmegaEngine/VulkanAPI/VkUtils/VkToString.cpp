#include "VkToString.h"

#include "utility/Logger.h"

#include <cassert>

namespace VulkanAPI
{
namespace VkUtils
{

bool isSamplerType(const std::string& type)
{
    return (type == "2D_Sampler" || type == "3D_Sampler" || type == "Cube_Sampler");
}

bool isBufferType(const std::string& type)
{
    return (type == "Uniform_Buffer" || type == "Storage_Buffer");
}

uint32_t vkTypeSize(const std::string& type)
{
    uint32_t size = 0;
    if (type == "float")
    {
        size = 4;
    }
    else if (type == "int")
    {
        size = 4;
    }
    else if (type == "uint")
    {
        size = 4;
    }
    else if (type == "vec2")
    {
        size = 8;
    }
    else if (type == "vec3")
    {
        size = 12;
    }
    else if (type == "vec4")
    {
        size = 16;
    }
    else if (type == "mat2")
    {
        size = 16;
    }
    else if (type == "mat3")
    {
        size = 32;
    }
    else if (type == "mat4")
    {
        size = 64;
    }
    else
    {
        LOGGER_INFO("Unrecognised glsl type. Size set to zero.");
    }
    return size;
}

void createVkShaderSampler(const std::string name, const std::string type, const uint16_t bind, const uint16_t setCount, std::string& output)
{
	assert(!type.empty());
	assert(!name.empty());
    
    std::string samplerTemplate = "layout (set = " + std::to_string(setCount) + ", binding = " + std::to_string(bind) +
    ") uniform ";
    
	if (type == "2D_Sampler")
	{
		output = samplerTemplate + "smapler2D " + name;
	}
	else if (type == "3D_Sampler")
    {
        output = samplerTemplate + "smapler3D " + name;
    }
    else if (type == "Cube_Sampler")
    {
        output = samplerTemplate + "smaplerCube " + name;
    }
}

void createVkShaderBuffer(const std::string name, const std::string type, std::vector<std::pair<std::string, std::string>>& items, const uint16_t bind, const uint16_t setCount, std::string& outputStr, uint32_t& outputSize)
{
    assert(!type.empty());
    assert(!name.empty());
    
    std::string bufferTemplate;
    if (type == "UniformBuffer")
    {
        bufferTemplate = "layout (set = " + std::to_string(setCount) + ", binding = " + std::to_string(bind) +
           ") uniform " + name + "\nstruct\n{\n";
    }
    else if (type == "StorageBuffer")
    {
        bufferTemplate = "layout (std140, binding = " + std::to_string(bind) +
        ") buffer " + name + "\nstruct\n{\n";
    }
    
    // keep a tally of the buffer size which will be used for creaing the descriptor set
    uint32_t bufferSize = 0;
    for (const auto& item : items)
    {
        bufferTemplate +=  "    " + item.second + " " + item.first + ";\n";
        bufferSize += vkTypeSize(item.second);
    }
    
    assert(bufferSize != 0);
    outputStr = bufferTemplate + "};";
    outputSize = bufferSize;
}

}    // namespace VkUtils
}    // namespace VulkanAPI
