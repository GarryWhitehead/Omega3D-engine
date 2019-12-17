#pragma once

#include "VulkanAPI/Common.h"

#include "utility/CString.h"

#include <shaderc/shaderc.hpp>

#include <optional>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace VulkanAPI
{
// forward decleartions
class VkContext;

class Shader
{

public:
    
    enum Type
    {
        Vertex,
        TesselationCon,
        TesselationEval,
        Geometry,
        Fragment,
        Compute,
		Count
    };

	Shader();
	~Shader();
    
    // not copyable
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    
    /**
    * Gathers the createInfo for all shaders into one blob in a format needed by the pipeline
     */
	vk::PipelineShaderStageCreateInfo& get()
	{
		return createInfo;
	}
    
	Sampler getSamplerType(std::string name);
    
    /**
     * @brief Takes a string of certain type and if valid, returns as a vulkan recognisible type.
       @param type A string of a valid type i.e. 'float', 'int', 'vec2', 'vec3', 'vec4'
       @param width The size of the type in bits - e.g. 32 for a float
     */
    static vk::Format Shader::getVkFormatFromType(std::string type, uint32_t width);
    
    /**
     * @brief Converts the StageType enum into a vulkan recognisible format
     */
	static vk::ShaderStageFlagBits getStageFlags(Shader::Type type);
    
    /**
     * @brief Derieves from the type specified, the stride in bytes
     */
    static uint32_t Shader::getStrideFromType(std::string type);
    
    /**
     * @brief compiles the specified code into glsl bytecode, and then creates
     * a shader module and createInfo ready for using with a vulkan pipeline
     */
	bool compile(VkContext& context, std::string shaderCode, const Shader::Type type);
    
	friend class ShaderProgram;

private:
    
    vk::ShaderModule module;
	Shader::Type type;
	vk::PipelineShaderStageCreateInfo createInfo;

};

class GlslCompiler
{
public:
    GlslCompiler(std::string shaderCode, const Shader::Type type);
    ~GlslCompiler();

    bool compile(bool optimise);

    void addVariant(Util::String variant, uint8_t value)
    {
        defines.insert(variant, value);
    }

    uint32_t* getData()
    {
        return output.data();
    }

    size_t getSize() const
    {
        return output.size();
    }
    
    using VariantMap = std::unordered_map<Util::String, uint8_t>;
    
private:
    std::vector<uint32_t> output;

    shaderc_shader_kind kind;
    std::string source;
    std::string sourceName;
    VariantMap defines;
};

} // namespace VulkanAPI
