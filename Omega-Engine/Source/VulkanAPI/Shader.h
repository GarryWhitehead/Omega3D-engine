#pragma once

#include "VulkanAPI/Common.h"
#include "libshaderc_util/file_finder.h"
#include "utility/CString.h"

#include <cstdint>
#include <optional>
#include <shaderc/shaderc.hpp>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
// forward decleartions
class VkContext;
class Sampler;

// This is a callback class to impelement shaderc's include interface
class IncludeInterface : public shaderc::CompileOptions::IncluderInterface
{
public:
    explicit IncludeInterface(const shaderc_util::FileFinder* fileFinder)
        : fileFinder(*fileFinder)
    {
    }

    ~IncludeInterface() override;

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type type,
        const char* requesting_source,
        size_t include_depth) override;

    // Releases an include result.
    void ReleaseInclude(shaderc_include_result* includeResult) override;

    // Returns a reference to the member storing the set of included files.
    const std::unordered_set<std::string>& filePathTrace() const
    {
        return includedFiles;
    }

private:

    // Used by GetInclude() to get the full filepath.
    const shaderc_util::FileFinder& fileFinder;

    // The full path and content of a source file.
    struct FileInfo
    {
        const std::string full_path;
        std::vector<char> contents;
    };

    // The set of full paths of included files.
    std::unordered_set<std::string> includedFiles;
};


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
        Count,
        Unknown
    };

    /**
     * @brief Information for passing variants to the shader and onto the compiler
     */
    struct VariantInfo
    {
        Util::String definition;
        uint8_t value;
        Shader::Type stage;
    };

    Shader(VkContext& context, const Type type);
    ~Shader();

    /**
     * Gathers the createInfo for all shaders into one blob in a format needed by the pipeline
     */
    vk::PipelineShaderStageCreateInfo& get()
    {
        return createInfo;
    }

    /**
     * @brief Takes a string of certain type and if valid, returns as a vulkan recognisible type.
       @param type A string of a valid type i.e. 'float', 'int', 'vec2', 'vec3', 'vec4'
       @param width The size of the type in bits - e.g. 32 for a float
     */
    static vk::Format getVkFormatFromType(std::string type, uint32_t width);

    /**
     * @brief Converts the StageType enum into a vulkan recognisible format
     */
    static vk::ShaderStageFlagBits getStageFlags(Shader::Type type);

    static Util::String shaderTypeToString(Shader::Type type);

    /**
     * @brief Derieves from the type specified, the stride in bytes
     */
    static uint32_t getStrideFromType(std::string type);

    /**
     * @brief compiles the specified code into glsl bytecode, and then creates
     * a shader module and createInfo ready for using with a vulkan pipeline
     */
    bool
    compile(std::string shaderCode, const Shader::Type type, std::vector<VariantInfo>& variants);

    friend class ShaderProgram;

private:
    VkContext& context;

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

    void addVariant(Util::String& variant, const uint8_t value)
    {
        definitions.emplace(variant.c_str(), value);
    }

    uint32_t* getData()
    {
        return output.data();
    }

    size_t getSize() const
    {
        return output.size();
    }

    size_t getByteSize() const
    {
        return output.size() * sizeof(uint32_t);
    }

    using VariantMap = std::unordered_map<const char*, uint8_t>;

private:
    std::vector<uint32_t> output;

    shaderc_shader_kind kind;
    std::string source;
    std::string sourceName;
    VariantMap definitions;

    // required for delaing with #include
    shaderc_util::FileFinder fileFinder;
};

} // namespace VulkanAPI
