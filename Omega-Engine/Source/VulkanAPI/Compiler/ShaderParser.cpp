#include "ShaderParser.h"

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"


namespace VulkanAPI
{

std::string ShaderParser::getErrorString()
{
    // TODO:: make this more verbose
    std::string result = "Shader compiling failed with error code: " +
        std::to_string(static_cast<int>(errorCache.code)) +
        "; at line: " + std::to_string(errorCache.lineNumber);

    return result;
}

// ================== ShaderParser =======================

Shader::Type ShaderParser::strToShaderType(std::string& str)
{
    Shader::Type result;
    if (str == "Vertex")
    {
        result = Shader::Type::Vertex;
    }
    else if (str == "Fragment")
    {
        result = Shader::Type::Fragment;
    }
    else if (str == "TesselationCon")
    {
        result = Shader::Type::TesselationCon;
    }
    else if (str == "TesselationEval")
    {
        result = Shader::Type::TesselationEval;
    }
    else if (str == "Geometry")
    {
        result = Shader::Type::Geometry;
    }
    else if (str == "Compute")
    {
        result = Shader::Type::Compute;
    }
    else
    {
        result = Shader::Type::Unknown;
    }
    return result;
}

ParserReturnCode ShaderParser::parseLine(
    const std::string line, ShaderDescriptor::TypeDescriptors& output, const uint8_t typeCount)
{
    std::string typeValues = line;

    // we assume that if the line has a colon-  it has a cmd line designator so remove it
    size_t pos = line.find(':');
    if (pos != std::string::npos)
    {
        typeValues = line.substr(pos + 1, line.size());
    }

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
            // clear to signal we have reached the end identifier
            typeValues.clear();
        }
        else
        {
            typeValues = typeValues.substr(pos + 1, typeValues.size());
        }

        temp = temp.substr(0, pos);

        pos = temp.find(' =');
        std::string id = temp.substr(0, pos);
        std::string value = temp.substr(pos + 1, temp.size());

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

ParserReturnCode ShaderParser::parseBuffer(uint32_t& idx, ShaderDescriptor::ItemDescriptors& output)
{
    assert(idx < buffer.size());

    constexpr uint8_t typeCount = 2;
    while (buffer[idx].find("#item:") != std::string::npos)
    {
        std::string& line = buffer[idx];

        // remove whitespace, newlines, etc. to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        if (line.empty() || line.find("//") != std::string::npos)
        {
            continue;
        }

        ParserReturnCode ret = parseLine(line, output, typeCount);
        if (ret != ParserReturnCode::Success)
        {
            errorCache = ParserErrorCache {idx, ret};
            return ret;
        }
        ++idx;
    }
    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parseShaderStage(uint32_t& idx)
{
    // Get the shader stage type from the string e.g. ##stage: Vertex
    size_t pos = buffer[idx].find_last_of(":");
    std::string stageStr = buffer[idx].substr(pos + 1, buffer[idx].size());

    Shader::Type stage = strToShaderType(stageStr);
    if (stage == Shader::Type::Unknown)
    {
        return ParserReturnCode::UnknownShaderType;
    }

    auto shader = std::make_unique<ShaderDescriptor>(stage);
    bool foundEndMarker = false;

    for (; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        // remove whitespace, newlines, etc. to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

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
        if (line.find("#input:") != std::string::npos)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->inputs.emplace_back(descr);
        }
        // output semantics - glsl code: layout (location = 0) out [TYPE] [NAME]
        else if (line.find("#output:") != std::string::npos)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->outputs.emplace_back(descr);
        }
        // speciliastion constants - should be preferred to the usual #define method
        else if (line.find("#constant:") != std::string::npos)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 3);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->constants.emplace_back(descr);
        }
        // push constants - one per shader stage supported - due to the size limit that can be
        // pushed, this shouldn't be an issue
        else if (line.find("#push_constant:") != std::string::npos)
        {
            ShaderDescriptor::BufferDescriptor constant;
            ParserReturnCode ret = parseLine(line, constant.descriptors, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            // this is required for compiling
            constant.descriptors.emplace_back(std::make_pair("Type", "PushConstant"));

            // parse each item.....
            uint32_t itemIdx = idx + 1;
            std::string itemLine = buffer[itemIdx];

            while (itemLine.find("#item:") != std::string::npos && itemIdx < buffer.size())
            {
                ShaderDescriptor::TypeDescriptors descr;
                ret = parseBuffer(itemIdx, descr);
                if (ret != ParserReturnCode::Success)
                {
                    return ret;
                }
                constant.items.emplace_back(descr);
                itemLine = buffer[++itemIdx];
            }
            shader->pConstants.emplace_back(constant);
            // bit nasty this, but will do for now as we have checked we are not overflowing the buffer
            idx = itemIdx - 1;
        }
        else if (line.find("#import_buffer:") != std::string::npos)
        {
            ShaderDescriptor::BufferDescriptor buffer;
            ParserReturnCode ret = parseLine(line, buffer.descriptors, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }

            ShaderDescriptor::TypeDescriptors descr;
            ret = parseBuffer(++idx, descr);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->ubos.emplace_back(buffer);
        }
        else if (line.find("#import_sampler:") != std::string::npos)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader->samplers.emplace_back(descr);
        }
        else if (line.find("#code_block:") != std::string::npos)
        {
            ++idx;
            bool foundCodeBlockEnd = false;
            while (idx < buffer.size())
            {
                if (buffer[idx].find("#end_code_block") != std::string::npos)
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
    if (!foundEndMarker)
    {
        return ParserReturnCode::MissingEndIdentifier;
    }

    descriptors.emplace_back(std::move(shader));

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

        // remove whitespace, newlines, etc. to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

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

    for (uint32_t idx = 0; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        // remove any whitespace, newlines, etc. now to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        if (line.empty() || line.find("//") != std::string::npos)
        {
            continue;
        }

        // check for each supported command
        if (line.find("##stage:") != std::string::npos)
        {
            ParserReturnCode ret = parseShaderStage(idx);
            if (ret != ParserReturnCode::Success)
            {
                errorCache = ParserErrorCache {idx, ret};
                return false;
            }
        }
        else if (line.find("##pipeline:") != std::string::npos)
        {
            ParserReturnCode ret = parsePipelineBlock(idx);
            if (ret != ParserReturnCode::Success)
            {
                errorCache = ParserErrorCache {idx, ret};
                return false;
            }
        }
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
