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
    return (type == "UniformBuffer" || type == "DynamicUniform" || type == "StorageBuffer");
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
        LOGGER_WARN("Type: %s; Unrecognised glsl type. Size set to zero.", type.c_str());
    }
    return size;
}

bool createVkShaderSampler(
    const std::string name,
    const std::string type,
    const uint16_t bind,
    const uint16_t setCount,
    std::string& output,
    uint32_t arraySize)
{
    assert(!type.empty());
    assert(!name.empty());

    std::string samplerTemplate = "layout (set = " + std::to_string(setCount) +
        ", binding = " + std::to_string(bind) + ") uniform ";

    if (type == "2D_Sampler")
    {
        output = samplerTemplate + "sampler2D ";
    }
    else if (type == "3D_Sampler")
    {
        output = samplerTemplate + "smapler3D ";
    }
    else if (type == "Cube_Sampler")
    {
        output = samplerTemplate + "samplerCube ";
    }
    else
    {
        LOGGER_ERROR(
            "Invalid sampler type: %s with name: %s found within 'Imports' block",
            type.c_str(),
            name.c_str());
        return false;
    }
    
    // check whether this is a sampler array...
    if (arraySize > 0)
    {
        output += "[" + std::to_string(arraySize) + "];\n";
    }
    else
    {
        output += name + ";";
    }
    
    
    return true;
}

bool createVkShaderBuffer(
    const std::string name,
    const std::string type,
    const std::vector<ShaderDescriptor::TypeDescriptors>& items,
    const uint16_t bind,
    const uint16_t setCount,
    std::string& outputStr,
    uint32_t& outputSize)
{
    assert(!type.empty());
    assert(!name.empty());

    std::string bufferTemplate;
    if (type == "UniformBuffer" || type == "DynamicUniform")
    {
        bufferTemplate = "layout (set = " + std::to_string(setCount) +
            ", binding = " + std::to_string(bind) + ") uniform " + name + "\n{\n";
    }
    else if (type == "StorageBuffer")
    {
        bufferTemplate =
            "layout (std140, binding = " + std::to_string(bind) + ") buffer " + name + "\n{\n";
    }
    else if (type == "PushConstant")
    {
        bufferTemplate = "layout (push_constant) uniform " + name + "\n{\n";
    }
    else
    {
        LOGGER_ERROR(
            "Invalid buffer type: %s with name: %s found within 'Imports' block",
            type.c_str(),
            name.c_str());
        return false;
    }

    // keep a tally of the buffer size which will be used for creaing the descriptor set
    uint32_t bufferSize = 0;
    for (const auto& item : items)
    {
        std::string itemName, itemType, offset;
        bool result = ShaderDescriptor::getTypeValue("Name", item, itemName);
        result &= ShaderDescriptor::getTypeValue("Type", item, itemType);
        if (!result)
        {
            return false;
        }
        
        // check for flags......
        std::string flags;
        bool usingExternal = false;
        
        if (ShaderDescriptor::getTypeValue("Flag", item, flags))
        {
            // external flags indicate that this item is using an external type. The only fallout from this is that we
            // cannot determine the buffer size.
            if (ShaderDescriptor::checkForFlag("External", flags))
            {
                usingExternal = true;
            }
        }
        
        // offsets are not mandatory
        if (ShaderDescriptor::getTypeValue("Offset", item, offset))
        {
            bufferTemplate += "\tlayout (offset = " + offset + ") ";
        }
        else
        {
            bufferTemplate += "\t";
        }
        
        // add the item type and name.....
        bufferTemplate += itemType + " " + itemName;
        
        // check whether this is an array
        std::string array;
        if (ShaderDescriptor::getTypeValue("Array_size", item, array))
        {
            bufferTemplate += "[" + array + "];\n";
        }
        else
        {
            bufferTemplate += ";\n";
        }
        
        if (!usingExternal)
        {
            bufferSize += vkTypeSize(itemType);
        }
    }

    outputStr = bufferTemplate + "}";
    outputSize = bufferSize;
    
    return true;
}

} // namespace VkUtils
} // namespace VulkanAPI
