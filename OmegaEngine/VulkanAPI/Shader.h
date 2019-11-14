#pragma once

#include "VulkanAPI/Common.h"

#include "utility/String.h"

#include <shaderc/shaderc.hpp>

#include <optional>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace VulkanAPI
{
// forward decleartions
class VkDriver;

class Shader
{

public:
    
    enum class StageType
    {
        Vertex,
        TesselationCon,
        TesselationEval,
        Geometry,
        Fragment,
        Compute,
    };

	Shader(VkDriver& context);
	~Shader();
    
    // not copyable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    
    /**
     * @brief The number of shaders
     */
	uint32_t shaderCount() const
	{
		return static_cast<uint32_t>(wrappers.size());
	}
    
    /**
    * Gathers the createInfo for all shaders into one blob in a format needed by the pipeline
     */
	vk::PipelineShaderStageCreateInfo *getData()
	{
		return wrappers.data();
	}
    
	Sampler getSamplerType(std::string name);
	vk::ImageLayout getImageLayout(std::string& name);
    
    /**
     * @brief Takes a string of certain type and if valid, returns as a vulkan recognisible type.
       @param type A string of a valid type i.e. 'float', 'int', 'vec2', 'vec3', 'vec4'
       @param width The size of the type in bits - e.g. 32 for a float
     */
    static vk::Format Shader::getVkFormatFromType(std::string type, uint32_t width);
    
    /**
     * @brief Converts the StageType enum into a vulkan recognisible format
     */
	static vk::ShaderStageFlagBits getStageFlags(Shader::StageType type);
    
    /**
     * @brief Derieves from the type specified, the stride in bytes
     */
    static uint32_t Shader::getStrideFromType(std::string type);
    
    /**
     * @brief Adds a shader module. This will compile the code into glsl bytecode, and then create
     * a shader module and createInfo ready for using with a vulkan pipeline
     */
    bool add(std::string shaderCode, const Shader::StageType type);
    
private:
    
    struct ShaderModuleInfo
    {
        vk::ShaderModule module;
        ShaderStageType type;
        vk::PipelineShaderStageCreateInfo createInfo;
    }

private:
    
	VkDriver& context;
    
    std::vector<ShaderModuleInfo> shaders;
};

class GlslCompiler
{
public:
    GlslCompiler(std::string shaderCode, const Shader::StageType type);
    ~GlslCompiler();

    bool compile(bool optimise);

    void addDefinition(Util::String define, uint8_t value)
    {
        defines.insert(define, value);
    }

    uint32_t* getData()
    {
        return output.data();
    }

    size_t getSize() const
    {
        return output.size();
    }

private:
    std::vector<uint32_t> output;

    shaderc_shader_kind kind;
    std::string source;
    std::string sourceName;
    std::unordered_map<Util::String, uint8_t> defines;
};

} // namespace VulkanAPI
