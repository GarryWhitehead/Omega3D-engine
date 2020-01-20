#pragma once

#include "VulkanAPI/Shader.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"

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

    /// inputs and outputs
    struct InOutDescriptor
    {
        std::string name;
        std::string type; //< the type of
    };
    
    struct SubDescriptor
    {
        std::string name;
        std::string type;
    };
    
    /// generic descriptor for different shader types
    struct Descriptor
    {
        std::string name;
        std::string type;
        std::string id; //< Buffers only - optional sub-name for a struct
        uint16_t groupId; //< specifies an explicit set number
        std::string variant; //< if set, specifies to inject a #ifdef statemnet
        std::string
            arrayConst; //< if set, specifies that this type is an array set by a constant value
        uint32_t arraySize = UINT32_MAX; //< if not uint32_max, indicates the type is an array
    };

    /// uniform buffers
    struct BufferDescriptor
    {
        Descriptor descr;
        std::vector<SubDescriptor> data;
    };

    /// Specialisation constants
    struct ConstantDescriptor
    {
        std::string name;
        std::string type;
        std::string value;
    };

    /// push constants
    struct PConstantDescriptor
    {
        std::string name;
        std::string type;
        std::string id;     // optional
        std::vector<SubDescriptor> data;
    };

    friend class ShaderCompiler;
    friend class ShaderParser;
    friend class ProgramManager;

private:
    // shader stage
    Shader::Type type;

    // sementic inputs and outputs
    std::vector<InOutDescriptor> inputs;
    std::vector<InOutDescriptor> outputs;

    // texture samplers to import; first: name, second: sampler type
    std::vector<Descriptor> samplers;

    // uniform buffers to import; first: name, second: buffer type
    std::vector<BufferDescriptor> ubos;

    // first: name, second: type, third: value
    std::vector<ConstantDescriptor> constants;
    std::vector<PConstantDescriptor> pConstants;

    std::vector<std::string> includeFiles;

    // the glsl code in text format
    std::string codePath;

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
    UnknownShaderType
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
    
    void parseRenderBlock(rapidjson::Document& doc);
    
    // grabs all the data from the string buffer ready for compiling
    bool parseShader();
    
    // parses an individual shader stage as stated by a ##stage: block
    ParserReturnCode parseShaderStage(const uint32_t startIdx);
    
    ParserReturnCode parseLine(const std::string type, const std::string line, std::string& output);
    
    friend class ShaderCompiler;

private:
    // used to work out the maximum set number across all stages
    uint16_t groupSize = 0;

    // This will be completed by the parser and ownership moved at compile time.
    // This is to stop having to compile this info twice when it can be easily done by the parser
    RenderStateBlock* renderState = nullptr;

    std::vector<std::unique_ptr<ShaderDescriptor>> descriptors;

    // path to the main glsl shader code
    std::string codePath;

    // input buffer
    std::vector<std::string> buffer;
};


} // namespace VulkanAPI
