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

/**
 * Compiles a parsed shder json file into data ready for inputting into the renderer
 */
class ShaderCompiler
{
public:
    ShaderCompiler(ShaderProgram& program, VkContext& context);
    ~ShaderCompiler();

    bool compile(ShaderParser& parser);

private:
    void prepareBindings(ShaderDescriptor* shader, ShaderBinding& binding, uint16_t& bind);

    void writeInputs(ShaderDescriptor* shader, ShaderDescriptor* nextShader);

    void prepareInputs(ShaderDescriptor* vertShader);

    void prepareOutputs(ShaderParser& compilerInfo);

    bool appendCodeBlocks(ShaderParser& compilerInfo);

    uint32_t findDesignator(std::vector<std::string>& buffer, uint32_t startIdx);

    Shader::Type getDesignatorStage(const std::string& line);

private:
    VkContext& context;

    /// variants to use when compiling the shader
    std::unordered_map<const char*, uint8_t> variants;

    /// the program which will be compiled too
    ShaderProgram& program;
};

} // namespace VulkanAPI
