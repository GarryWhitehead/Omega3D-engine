#include "ShaderParser.h"

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"


namespace VulkanAPI
{

// ================== ShaderParser =======================

ParserReturnCode ShaderParser::parseLine(
    const std::string line, ShaderDescriptor::TypeDescriptors& output, const uint8_t typeCount)
{
    // all cmd designators end with a colon - use this to remove the main-type identifier
    size_t pos = line.find(':');
    if (pos == std::string::npos)
    {
        return ParserReturnCode::InvalidLine;
    }

    std::string typeValues = line.substr(0, pos);

    // now use the id=value, format to extract the data.
    while (typeValues.find('=') != std::string::npos)
    {
        std::string temp = typeValues;
        pos = temp.find_first_of(',');
        if (pos == std::string::npos)
        {
            // could be the end of the line - check for a ;
            pos = temp.find_first_of(';');
            if (pos == std::string::npos)
            {
                return ParserReturnCode::MissingSemiColon;
            }
        }
        temp = temp.substr(pos, temp.size());

        pos = temp.find('=');
        std::string id = temp.substr(pos, temp.size());
        std::string value = temp.substr(0, pos);

        // remove any whitespace form both before finishing
        id.erase(remove_if(id.begin(), id.end(), isspace), id.end());
        value.erase(remove_if(value.begin(), value.end(), isspace), value.end());
        output.emplace_back(std::make_pair(id, value));
    }

    // final check to make sure that the required number of types matches with our output vector
    // this is not a concreate check as some will be optional and we may have misseed the mandatory
    // types!! a more conclusive check is carried out during compile time
    if (output.size() < typeCount)
    {
        return ParserReturnCode::IncorrectTypeCount;
    }

    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parseBuffer(size_t& idx, ShaderDescriptor::ItemDescriptors& output)
{
    assert(idx < buffer.size());

    constexpr uint8_t typeCount = 2;
    while (buffer[idx].find("#item:") != std::string::npos)
    {
        if (buffer[idx].empty() || buffer[idx].find("//") != std::string::npos)
        {
            continue;
        }

        ParserReturnCode ret = parseLine(buffer[idx++], output, typeCount);
        if (ret != ParserReturnCode::Success)
        {
            return ret;
        }
    }
    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parseShaderStage(uint32_t& idx)
{
    // Get the shader stage type from the string e.g. ##stage: Vertex
    size_t pos = buffer[idx].find_last_of(":");
    std::string stageStr = buffer[idx].substr(pos, buffer[idx].size());
    stageStr.erase(
        remove_if(stageStr.begin(), stageStr.end(), isspace), stageStr.end()); // remove whitespace

    Shader::Type stage = Shader::strToShaderType(stageStr);
    if (stage == Shader::Type::Unknown)
    {
        return ParserReturnCode::UnknownShaderType;
    }

    auto shader = std::make_unique<ShaderDescriptor>(stage);
    bool foundEndMarker = false;

    for (; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        if (line.empty() || line.find("//") != std::string::npos)
        {
            continue;
        }

        if (line.find("##end_stage") != std::string::npos)
        {
            foundEndMarker = true;
            break;
        }

        // TODO: lots of duplicated code here - refactor this with a templated function at some
        // point check for each supported stage variable type - must begin with '#' input semantics
        // - glsl code: layout (location = 0) in [TYPE] [NAME]
        if (line.find("#inputs:") != std::string::npos)
        {
            ShaderDescriptor::InputDescriptor input;
            ParserReturnCode ret = parseLine(line, input.descriptors, input.DescrSize);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->inputs.emplace_back(input);
        }
        // output semantics - glsl code: layout (location = 0) out [TYPE] [NAME]
        else if (line.find("#output:") != std::string::npos)
        {
            ShaderDescriptor::OutputDescriptor output;
            ParserReturnCode ret = parseLine(line, output.descriptors, output.DescrSize);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->outputs.emplace_back(output);
        }
        // speciliastion constants - should be preferred to the usual #define method
        else if (line.find("#constant:") != std::string::npos)
        {
            ShaderDescriptor::ConstantDescriptor constant;
            ParserReturnCode ret = parseLine(line, constant.descriptors, constant.DescrSize);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->constants.emplace_back(constant);
        }
        // push constants - one per shader stage supported - due to the size limit that can be
        // pushed, this shouldn't be an issue
        else if (line.find("#push_constant:") != std::string::npos)
        {
            ShaderDescriptor::PConstantDescriptor constant;
            ParserReturnCode ret = parseLine(line, constant.descriptors, constant.DescrSize);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            // this is required for compiling
            constant.descriptors.emplace_back(std::make_pair("Type", "PushConstant"));
            ret = parseBuffer(++idx, constant.items);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->pConstants.emplace_back(constant);
        }
        else if (line.find("#import_buffer:") != std::string::npos)
        {
            ShaderDescriptor::BufferDescriptor buffer;
            ParserReturnCode ret = parseLine(line, buffer.descriptors, buffer.DescrSize);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            ret = parseBuffer(++idx, buffer.items);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->ubos.emplace_back(buffer);
        }
        else if (line.find("#import_sampler:") != std::string::npos)
        {
            ShaderDescriptor::SamplerDescriptor sampler;
            ParserReturnCode ret = parseLine(line, sampler.descriptors, sampler.DescrSize);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->ubos.emplace_back(buffer);
        }
        else if (line.find("#code_block:") != std::string::npos)
        {
            ++idx;
            bool foundCodeBlockEnd = false;
            while (idx < buffer.size())
            {
                if (buffer[idx].find("##end_code_block") != std::string::npos)
                {
                    foundCodeBlockEnd = true;
                    break;
                }
                shader->codeBlock.append(buffer[idx++]);
            }
            if (!foundCodeBlockEnd)
            {
                return ParserReturnCode::MissingCodeBlockEnd;
            }
        }
    }

    return ParserReturnCode::Success;
}

void ShaderParser::addStage(ShaderDescriptor* shader)
{
    descriptors.emplace_back(shader);
}

ParserReturnCode ShaderParser::parsePipelineBlock(uint32_t& idx)
{
    bool foundEndMarker = false;

    for (; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        if (line.empty() || line.find("//") != std::string::npos)
        {
            continue;
        }

        if (line.find("##end_pipeline") != std::string::npos)
        {
            foundEndMarker = true;
            break;
        }

        ParserReturnCode ret = parseLine(line, pipelineDescrs, 0);
        if (ret != ParserReturnCode::Success)
        {
            return ret;
        }
    }

    if (!foundEndMarker)
    {
        return ParserReturnCode::MissingEndIdentifier;
    }
    return ParserReturnCode::Success;
}

bool ShaderParser::parseShader()
{
    uint32_t idx = 0;

    for (const std::string& line : buffer)
    {
        if (line.empty() || line.find("//") != std::string::npos)
        {
            continue;
        }

        // check for each supported command
        if (line.find("##stage:") != std::string::npos)
        {
            if (parseShaderStage(idx) != ParserReturnCode::Success)
            {
                // deal with parse code
                return false;
            }
        }
        else if (line.find("##pipeline:") != std::string::npos)
        {
            if (parsePipelineBlock(idx) != ParserReturnCode::Success)
            {
                // deal with parse code
                return false;
            }
        }
        ++idx;
    }

    return true;
}

bool ShaderParser::loadAndParse(Util::String filename)
{
    // we use the definition from cmake for the shader path. Will add a user override for this at
    // some point
    Util::String absPath = Util::String::append(Util::String(OE_SHADER_DIR), filename);

    if (!FileUtil::readFileIntoBuffer(absPath.c_str(), buffer))
    {
        return false;
    }

    if (!parseShader())
    {
        return false;
    }

    return true;
}

bool ShaderParser::loadAndParse(Util::String filename, ShaderDescriptor* shader, Shader::Type type)
{
    std::string shaderBuffer;
    if (!FileUtil::readFileIntoBuffer(filename.c_str(), buffer))
    {
        return false;
    }

    Util::String vertexId = Shader::shaderTypeToString(type);
    uint16_t maxGroup = 0;

    /*if (!readShader(doc, *shader, vertexId, maxGroup))
    {
        LOGGER_ERROR(
            "Unable to read shader block from json file; filename = %s.", filename.c_str());
        return false;
    }*/

    return true;
}

} // namespace VulkanAPI
