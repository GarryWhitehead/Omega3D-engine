#pragma once

#include "VulkanAPI/Shader.h"

#include <cstdint>
#include <regex>
#include <string>
#include <vector>

namespace VulkanAPI
{
// forward declerations
struct RenderStateBlock;
class ProgramManager;

class ShaderDescriptor
{
public:
    ShaderDescriptor() = delete;

    ShaderDescriptor(Shader::Type type) : type(type)
    {
    }

    using Descriptor = std::pair<std::string, std::string>;
    using TypeDescriptors = std::vector<Descriptor>;
    using ItemDescriptors = std::vector<Descriptor>;

    /// buffers
    struct BufferDescriptor
    {
        static const uint8_t DescrSize = 2;
        TypeDescriptors descriptors;
        ItemDescriptors items;
    };

    // extracts a value from a type descriptor pair
    template <typename T>
    static bool getTypeValue(const std::string type, const TypeDescriptors descr, T& out)
    {
        if (descr.empty())
        {
            return false;
        }

        auto iter = std::find_if(descr.begin(), descr.end(), [&type](const Descriptor& lhs) {
            return lhs.first == type;
        });

        if (iter == descr.end())
        {
            return false;
        }

        auto isNumber = [](const std::string& str) -> bool {
            return std::regex_match(str, std::regex("(\\+|-)?[0-9]*(\\.?([0-9]+))$"));
        };

        std::string value = iter->second;
        if constexpr (std::is_same_v<T, float>)
        {
            if (!isNumber(value))
            {
                out = std::stof(value);
            }
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            if (!isNumber(value))
            {
                out = std::stoi(value);
            }
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            out = value;
        }
        else
        {
            return false;
        }
        return true;
    }

    friend class ShaderCompiler;
    friend class ShaderParser;
    friend class ProgramManager;

private:
    // shader stage
    Shader::Type type;

    // sementic inputs and outputs
    TypeDescriptors inputs;
    TypeDescriptors outputs;

    // texture samplers to import; first: name, second: sampler type
    TypeDescriptors samplers;

    // uniform buffers to import; first: name, second: buffer type
    std::vector<BufferDescriptor> ubos;

    // first: name, second: type, third: value
    TypeDescriptors constants;
    std::vector<BufferDescriptor> pConstants;

    std::vector<std::string> includeFiles;

    // the glsl code in text format
    std::string codeBlock;

    // variants for this stage
    GlslCompiler::VariantMap variants;

    // used by the compiler to prepare the code block for inputs, etc.
    std::string appendBlock;
};

enum class ParserReturnCode
{
    Success,
    NotFound,
    InvalidTypeFormat,
    MissingStageEnd,
    UnknownShaderType,
    InvalidLine,
    MissingSemiColon,
    IncorrectTypeCount,
    MissingCodeBlockEnd,
    MissingEndIdentifier
};

/**
 * Raw data obtained from a json sampler file.
 */
class ShaderParser
{
public:
    ShaderParser() = default;

    /**
     * @brief Loads a shader json file into a string buffer and parses the json file to extract all
     * data ready for compiling
     * @param filename The path to the shader json to load
     * @param output The string buffer in which the json file will be contained within
     */
    bool loadAndParse(Util::String filename);

    /**
     * @brief Takes a empty shader descriptor and parses form the specified json file and shader
     * type, the relevant data This function is solely used when using wanting to merge different
     * shader stages from different files. Usually this will be cached with the shader manager.
     */
    bool loadAndParse(Util::String filename, ShaderDescriptor* shader, Shader::Type type);

    void addStage(ShaderDescriptor* shader);

private:
    ParserReturnCode parsePipelineBlock(uint32_t& idx);

    // grabs all the data from the string buffer ready for compiling
    bool parseShader();

    // parses an individual shader stage as stated by a ##stage: block
    ParserReturnCode parseShaderStage(uint32_t& idx);

    ParserReturnCode parseLine(
        const std::string line, ShaderDescriptor::TypeDescriptors& output, const uint8_t typeCount);

    ParserReturnCode parseBuffer(size_t& idx, ShaderDescriptor::ItemDescriptors& output);

    friend class ShaderCompiler;

private:
    // used to work out the maximum set number across all stages
    uint16_t groupSize = 0;

    ShaderDescriptor::TypeDescriptors pipelineDescrs;

    std::vector<std::unique_ptr<ShaderDescriptor>> descriptors;

    // input buffer
    std::vector<std::string> buffer;
};


} // namespace VulkanAPI