#pragma once

#include "VulkanAPI/Shader.h"

#include <cstdint>
#include <unordered_map>

namespace VulkanAPI
{
// forward declerations
class ShaderDescriptor;
class ShaderProgram;
class ShaderBinding;
class ShaderParser;
class VkContext;

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
    InvalidPipelineType
};

/**
 * Compiles a parsed shder json file into data ready for inputting into the renderer
 */
class ShaderCompiler
{
public:
    ShaderCompiler(ShaderProgram& program, VkContext& context);
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

    CompilerReturnCode compileAll(ShaderParser& parser);

    CompilerReturnCode
    prepareBindings(ShaderDescriptor* shader, ShaderBinding& binding);

    CompilerReturnCode writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader);

    CompilerReturnCode prepareInputs(ShaderDescriptor* vertShader);

    CompilerReturnCode prepareOutputs(ShaderParser& compilerInfo);

    CompilerReturnCode preparePipelineBlock(ShaderParser& compilerInfo);

    bool getBool(std::string type);

    static void printShaderCode(const std::string& block);

    uint8_t getCurrentBinding(const uint8_t groupId);

private:
    VkContext& context;

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
