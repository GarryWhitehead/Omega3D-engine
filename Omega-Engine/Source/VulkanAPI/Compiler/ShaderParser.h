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

#include "VulkanAPI/Shader.h"
#include "utility/Logger.h"

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
    ShaderDescriptor() = default;

    ShaderDescriptor(Shader::Type type);
    ~ShaderDescriptor() = default;

    using Descriptor = std::pair<std::string, std::string>;
    using TypeDescriptors = std::vector<Descriptor>;
    using ItemDescriptors = std::vector<Descriptor>;

    /// buffers
    struct BufferDescriptor
    {
        TypeDescriptors descriptors;
        std::vector<ItemDescriptors> items;
    };

    // extracts a value from a type descriptor pair
    template <typename T>
    static bool getTypeValue(const std::string type, const TypeDescriptors descr, T& out);

    static bool hasDescriptor(std::string id, std::vector<TypeDescriptors>& descrs);
    static bool hasDescriptorValue(std::string value, std::vector<TypeDescriptors>& typeDescrs);
    static Descriptor* findDescriptor(std::string id, std::vector<TypeDescriptors>& descrs);

    static bool hasId(std::string id, ItemDescriptors& descrs);
    static Descriptor* findId(std::string id, ItemDescriptors& descrs);

    static bool checkForFlag(std::string flag, std::string line);

    friend class ShaderCompiler;
    friend class ShaderParser;
    friend class ProgramManager;

private:
    // shader stage
    Shader::Type type;

    // sementic inputs and outputs
    std::vector<TypeDescriptors> inputs;
    std::vector<TypeDescriptors> outputs;

    // texture samplers to import; first: name, second: sampler type
    std::vector<TypeDescriptors> samplers;
    
    // special case for materials - they aren't processed and the the info only used for layout
    std::vector<TypeDescriptors> materialSamplers;
    
    // uniform buffers to import; first: name, second: buffer type
    std::vector<BufferDescriptor> ubos;

    // first: name, second: type, third: value
    std::vector<TypeDescriptors> constants;
    std::vector<BufferDescriptor> pConstants;

    std::vector<std::string> includeFiles;

    // the glsl code in text format
    std::string codeBlock;

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
    MissingEndIdentifier,
    BufferHasNoItems,
    MissingBufferStartMarker,
    MissingBufferEndMarker,
    InvalidConstantForArray,
    InvalidCommand
};

/**
 * Raw data obtained from a json sampler file.
 */
class ShaderParser
{
public:

    constexpr static size_t EOL = -1;

    ShaderParser() = default;

    /**
     * @brief Loads a shader json file into a string buffer and parses the json file to extract all
     * data ready for compiling
     * @param filename The path to the shader json to load
     * @param output The string buffer in which the json file will be contained within
     */
    bool loadAndParse(Util::String filename);

    bool addStage(ShaderDescriptor& shader);

    ShaderDescriptor* getShaderDescriptor(const Shader::Type type);

    static Shader::Type strToShaderType(std::string& str);

    std::string getErrorString();
    
private:
    struct ParserErrorCache
    {
        uint32_t lineNumber;
        ParserReturnCode code;
    };

    ParserReturnCode parsePipelineBlock(uint32_t& idx);

    // grabs all the data from the string buffer ready for compiling
    // if a specific shader descriptor is specified, then only that stage will be parsed 
    bool parseShader();

    // parses an individual shader stage as stated by a ##stage: block
    ParserReturnCode parseShaderStage(uint32_t& idx, ShaderDescriptor& shader);

    ParserReturnCode parseLine(
        const std::string line, ShaderDescriptor::TypeDescriptors& output, const uint8_t typeCount);

    ParserReturnCode parseBuffer(uint32_t& idx, ShaderDescriptor::BufferDescriptor& output);

    ParserReturnCode parseIncludeFile(const std::string line, std::string& output);

    ParserReturnCode
    debugBuffer(ShaderDescriptor::BufferDescriptor& buffer, ShaderDescriptor& shader);

    friend class ShaderCompiler;

private:

    Util::String shaderId;

    // used to work out the maximum set number across all stages
    uint16_t groupSize = 0;

    ShaderDescriptor::TypeDescriptors pipelineDescrs;

    std::vector<ShaderDescriptor> descriptors;

    // input buffer
    std::vector<std::string> buffer;

    // for error handling only
    ParserErrorCache errorCache;
};

template <typename T>
bool ShaderDescriptor::getTypeValue(const std::string type, const TypeDescriptors descr, T& out)
{
    if (descr.empty())
    {
        return false;
    }

    auto iter = std::find_if(
        descr.begin(), descr.end(), [&type](const Descriptor& lhs) { return lhs.first == type; });

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
        if (isNumber(value))
        {
            out = std::stof(value);
        }
    }
    else if constexpr (std::is_same_v<T, int>)
    {
        if (isNumber(value))
        {
            out = std::stoi(value);
        }
    }
    else if constexpr (std::is_same_v<T, uint16_t>)
    {
        if (isNumber(value))
        {
            out = std::stoi(value);
        }
    }
    else if constexpr (std::is_same_v<T, std::string>)
    {
        out = value;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        if (value == "True")
        {
            out = true;
        }
        else if (value == "False")
        {
            out = false;
        }
        else
        {
            return false;
        }
    }
    else
    {
        LOGGER_WARN("Unrecognised type found whilst compiling shader; type name: %s", type.c_str());
        return false;
    }
    return true;
}
} // namespace VulkanAPI
