/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Shader.h"

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

    CompilerReturnCode prepareBindings(
        uint32_t shaderId, ShaderDescriptor& shader, ShaderBinding& binding, uint16_t& maxSetCount);

    CompilerReturnCode writeInputs(ShaderDescriptor& shader, ShaderDescriptor& nextShader);

    CompilerReturnCode prepareVertexInputs(ShaderDescriptor& vertShader);

    CompilerReturnCode prepareOutputs(ShaderParser& compilerInfo);

    CompilerReturnCode preparePipelineBlock(ShaderParser& compilerInfo);

    bool getBool(std::string type);
    uint32_t getInt(std::string value);

    static void printShaderCode(const std::string& block);

    uint8_t getCurrentBinding(const uint8_t groupId);

    CompilerReturnCode prepareImport(
        ShaderDescriptor& shader,
        ImportType type,
        ShaderDescriptor::TypeDescriptors& descr,
        ImportInfo& output,
        std::vector<ShaderDescriptor::ItemDescriptors> items = {});

    bool checkVariantStatus(const std::string& variant);

private:
    VkDriver& driver;

    // the current binding for each set currently active <set, bind>
    std::unordered_map<uint8_t, uint8_t> currentBinding;

    // the program which will be compiled too
    ShaderProgram& program;

    // for error handling purposes only
    CompilerErrorCache errorCache;
};

} // namespace VulkanAPI
