#pragma once

#include "VulkanAPI/Shader.h"

#include "VulkanAPI/Compiler/ShaderParser.h"

#include <cstdint>
#include <unordered_map>

namespace VulkanAPI
{
// forward declerations
class ShaderProgram;
class ShaderBinding;
class VkDriver;

enum class CompilerReturnCode
{
    Success,
    InvalidOutput,
    InvalidInput,
    InvalidPushConstant,
    InvalidConstant,
    InvalidSampler,
    InvalidBuffer,
    MissingCodeBlock,
    ErrorCompilingGlsl,
    InvalidPipelineType,
    MissingNameOrType,
    InvalidMaterialInput
};

/**
 * Compiles a parsed shder json file into data ready for inputting into the renderer
 */
class ShaderCompiler
{
public:
    ShaderCompiler(ShaderProgram& program, VkDriver& driver);
    ~ShaderCompiler();

    bool compile(ShaderParser& parser);

    std::string getErrorString();

private:
    struct CompilerErrorCache
    {
        std::string name;
        std::string type;
        CompilerReturnCode code;
    };

    enum class ImportType
    {
        Buffer,
        Sampler,
        PushConstant
    };

    struct ImportInfo
    {
        // mandatory
        std::string name;
        std::string type;
        // optional
        std::string subId;
        std::string variant;
        uint16_t groupId = 0;
        uint8_t bind = 0;
        uint32_t bufferSize = 0;
    };

    CompilerReturnCode compileAll(ShaderParser& parser);

    CompilerReturnCode prepareBindings(uint32_t shaderId, ShaderDescriptor& shader, ShaderBinding& binding, uint16_t& maxSetCount);

    CompilerReturnCode writeInputs(ShaderDescriptor& shader, ShaderDescriptor& nextShader);

    CompilerReturnCode prepareVertexInputs(ShaderDescriptor& vertShader);

    CompilerReturnCode prepareOutputs(ShaderParser& compilerInfo);

    CompilerReturnCode preparePipelineBlock(ShaderParser& compilerInfo);

    bool getBool(std::string type);

    static void printShaderCode(const std::string& block);

    uint8_t getCurrentBinding(const uint8_t groupId);

    CompilerReturnCode prepareImport(
        ShaderDescriptor& shader,
        ImportType type,
        ShaderDescriptor::TypeDescriptors& descr,
        ImportInfo& output,
        std::vector<ShaderDescriptor::ItemDescriptors> items = {});

private:
    VkDriver& driver;

    // variants to use when compiling the shader
    std::unordered_map<const char*, uint8_t> variants;

    // the current binding for each set currently active <set, bind>
    std::unordered_map<uint8_t, uint8_t> currentBinding;

    // the program which will be compiled too
    ShaderProgram& program;

    // for error handling purposes only
    CompilerErrorCache errorCache;
};

} // namespace VulkanAPI
