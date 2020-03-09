#include "ShaderParser.h"

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"


namespace VulkanAPI
{

ShaderDescriptor::ShaderDescriptor(Shader::Type type) : type(type)
{
}

bool ShaderDescriptor::hasDescriptor(std::string id, std::vector<TypeDescriptors>& typeDescrs)
{
    for (const TypeDescriptors& typeDescr : typeDescrs)
    {
        for (const Descriptor& descr : typeDescr)
        {
            if (descr.first == id)
            {
                return true;
            }
        }
    }
    return false;
}

bool ShaderDescriptor::hasDescriptorValue(
    std::string value, std::vector<TypeDescriptors>& typeDescrs)
{
    for (const TypeDescriptors& typeDescr : typeDescrs)
    {
        for (const Descriptor& descr : typeDescr)
        {
            if (descr.second == value)
            {
                return true;
            }
        }
    }
    return false;
}

ShaderDescriptor::Descriptor*
ShaderDescriptor::findDescriptor(std::string id, std::vector<TypeDescriptors>& typeDescrs)
{
    for (TypeDescriptors& typeDescr : typeDescrs)
    {
        for (Descriptor& descr : typeDescr)
        {
            if (descr.first == id)
            {
                return &descr;
            }
        }
    }
    return nullptr;
}

bool ShaderDescriptor::hasId(std::string id, ItemDescriptors& descrs)
{

    for (const Descriptor& descr : descrs)
    {
        if (descr.first == id)
        {
            return true;
        }
    }
    return false;
}

ShaderDescriptor::Descriptor* ShaderDescriptor::findId(std::string id, ItemDescriptors& descrs)
{
    uint32_t count = 0;

    for (Descriptor& descr : descrs)
    {
        if (descr.first == id)
        {
            return &descr;
        }
    }
    ++count;

    return nullptr;
}

bool ShaderDescriptor::checkForFlag(std::string flag, std::string line)
{
    // flags are split by the + denoter
    if (line.find(flag) != std::string::npos)
    {
        return true;
    }
    return false;
}

// ==========================================================

std::string ShaderParser::getErrorString()
{
    // TODO:: make this more verbose
    std::string result = "Shader parsing failed with error code: " +
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
    if (pos != EOL)
    {
        typeValues = line.substr(pos + 1, line.size());
    }

    // now use the id=value, format to extract the data.
    bool haveFlag = false;
    while (typeValues.find('=') != EOL)
    {
        std::string temp = typeValues;
        pos = temp.find_first_of(',');
        if (pos == EOL)
        {
            // could be the end of the line - check for a ;
            pos = temp.find_first_of(';');
            if (pos == EOL)
            {
                return ParserReturnCode::MissingSemiColon;
            }
            // clear to signal we have reached the end identifier and the break on the next loop
            typeValues.clear();
        }
        else
        {
            // remove the id-value pair that is processed this iteration so we don't duplicate
            typeValues = typeValues.substr(pos + 1, typeValues.size());
        }
        // discard everything else except for the current id-value pair
        temp = temp.substr(0, pos);

        pos = temp.find('=');
        std::string id = temp.substr(0, pos);
        std::string value = temp.substr(pos + 1, temp.size());

        // check if the id has a special flag as denoted by {}
        if (value.find('{') != EOL && value.find('}') != EOL)
        {
            std::string flag = value;
            size_t start_pos = flag.find('{');
            flag = flag.substr(start_pos + 1, flag.size());

            // isolate the flag
            size_t end_pos = flag.find('}');
            flag = flag.substr(0, end_pos);

            // remove the flag from the id
            end_pos = value.find('}');
            value = value.substr(end_pos + 1, value.size());

            // add the flag to the descriptors
            // if a flag already exsists, merge with that one
            if (haveFlag)
            {
                ShaderDescriptor::Descriptor* descr = ShaderDescriptor::findId("Flag", output);
                assert(descr);
                descr->second += "+" + flag;
            }
            else
            {
                output.emplace_back(std::make_pair("Flag", flag));
                haveFlag = true;
            }
        }

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

ParserReturnCode
ShaderParser::parseBuffer(uint32_t& idx, ShaderDescriptor::BufferDescriptor& output)
{
    assert(idx < buffer.size());

    // read the buffer items into a temp buffer for easier parsing. Must begin and end in '[[' /
    // ']]'
    constexpr uint8_t typeCount = 2;
    std::vector<std::string> tempBuffer;

    bool foundStartMarker = false;
    bool foundEndMarker = false;

    // find the start
    while (idx < buffer.size())
    {
        if (buffer[idx++].find("[[") != EOL)
        {
            foundStartMarker = true;
            break;
        }
    }
    if (!foundStartMarker)
    {
        return ParserReturnCode::MissingBufferStartMarker;
    }

    // find the end
    while (idx < buffer.size())
    {
        if (buffer[idx].find("]]") != EOL)
        {
            foundEndMarker = true;
            break;
        }

        // remove whitespace, newlines, etc. as we go
        std::string line = buffer[idx++];
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());
        tempBuffer.emplace_back(line);
    }
    if (!foundStartMarker)
    {
        return ParserReturnCode::MissingBufferEndMarker;
    }

    // check we actually have items to process...
    if (tempBuffer.empty())
    {
        return ParserReturnCode::BufferHasNoItems;
    }

    for (const std::string& line : tempBuffer)
    {
        ShaderDescriptor::TypeDescriptors descr;
        ParserReturnCode ret = parseLine(line, descr, typeCount);
        if (ret != ParserReturnCode::Success)
        {
            errorCache = ParserErrorCache {idx, ret};
            return ret;
        }
        output.items.emplace_back(descr);
    }

    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parseIncludeFile(const std::string line, std::string& output)
{
    size_t pos = line.find(':');
    if (pos != EOL)
    {
        output = line.substr(pos + 1, line.size());
    }
    else
    {
        return ParserReturnCode::InvalidLine;
    }
    size_t first_pos = output.find_first_of('"');
    if (first_pos == EOL)
    {
        return ParserReturnCode::InvalidLine;
    }
    output = output.substr(first_pos + 1, output.size());

    size_t last_pos = output.find_last_of('"');
    if (last_pos == EOL)
    {
        return ParserReturnCode::InvalidLine;
    }
    output = output.substr(0, last_pos);

    return ParserReturnCode::Success;
}

ParserReturnCode
ShaderParser::debugBuffer(ShaderDescriptor::BufferDescriptor& buffer, ShaderDescriptor& shader)
{
    // if array size has specified and the size is based on a constant value then check this
    // constant exsists.
    for (ShaderDescriptor::ItemDescriptors& descrs : buffer.items)
    {
        if (ShaderDescriptor::hasId("Array_size", descrs) &&
            ShaderDescriptor::hasId("Flag", descrs))
        {
            ShaderDescriptor::Descriptor* descr = ShaderDescriptor::findId("Flag", descrs);
            assert(descr);

            if (ShaderDescriptor::checkForFlag("constant", descr->second))
            {
                ShaderDescriptor::Descriptor* arrayDescr =
                    ShaderDescriptor::findId("Array_size", descrs);
                std::string constantValue = arrayDescr->second;
                if (!ShaderDescriptor::hasDescriptorValue(constantValue, shader.constants))
                {
                    return ParserReturnCode::InvalidConstantForArray;
                }
            }
        }
    }
    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parseShaderStage(uint32_t& idx, ShaderDescriptor& shader)
{
    bool foundEndMarker = false;

    for (; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        // remove whitespace, newlines, etc. to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        if (line.empty() || line.find("//") != EOL)
        {
            continue;
        }

        if (line.find("##end_stage") != EOL)
        {
            foundEndMarker = true;
            break;
        }

        // TODO: lots of duplicated code here - refactor this with a templated function at some
        // point check for each supported stage variable type - must begin with '#' input semantics
        if (line.find("#include_file:") != std::string::npos)
        {
            std::string include;
            ParserReturnCode ret = parseIncludeFile(line, include);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.includeFiles.emplace_back(include);
        }
        // - glsl code: layout (location = 0) in [TYPE] [NAME]
        else if (line.find("#input:") != EOL)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.inputs.emplace_back(descr);
        }
        // output semantics - glsl code: layout (location = 0) out [TYPE] [NAME]
        else if (line.find("#output:") != EOL)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.outputs.emplace_back(descr);
        }
        // speciliastion constants - should be preferred to the usual #define method
        else if (line.find("#constant:") != EOL)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 3);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.constants.emplace_back(descr);
        }
        // push constants - one per shader stage supported - due to the size limit that can be
        // pushed, this shouldn't be an issue
        else if (line.find("#push_constant:") != EOL)
        {
            ShaderDescriptor::BufferDescriptor constant;
            ParserReturnCode ret = parseLine(line, constant.descriptors, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            // this is required for compiling
            constant.descriptors.emplace_back(std::make_pair("Type", "PushConstant"));

            // collect all items
            ret = parseBuffer(++idx, constant);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            if (constant.items.empty())
            {
                return ParserReturnCode::BufferHasNoItems;
            }

            shader.pConstants.emplace_back(constant);
        }
        else if (line.find("#import_buffer:") != EOL)
        {
            ShaderDescriptor::BufferDescriptor buffer;
            ParserReturnCode ret = parseLine(line, buffer.descriptors, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }

            // collect all items
            ret = parseBuffer(++idx, buffer);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            if (buffer.items.empty())
            {
                return ParserReturnCode::BufferHasNoItems;
            }

            // do a debug check if certain ids' are present and flags are set
            ret = debugBuffer(buffer, shader);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }

            shader.ubos.emplace_back(buffer);
        }
        else if (line.find("#import_sampler:") != EOL)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.samplers.emplace_back(descr);
        }
        else if (line.find("#import_material_sampler:") != EOL)
        {
            ShaderDescriptor::TypeDescriptors descr;
            ParserReturnCode ret = parseLine(line, descr, 2);
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.materialSamplers.emplace_back(descr);
        }
        else if (line.find("#code_block:") != EOL)
        {
            ++idx;
            bool foundCodeBlockEnd = false;
            while (idx < buffer.size())
            {
                if (buffer[idx].find("#end_code_block") != EOL)
                {
                    foundCodeBlockEnd = true;
                    break;
                }
                shader.codeBlock.append(buffer[idx++]);
            }
            if (!foundCodeBlockEnd)
            {
                return ParserReturnCode::MissingCodeBlockEnd;
            }
        }
        else
        {
            // if the line has a hash but the command isn't recognised, then this is an error
            if (line.find('#') != EOL)
            {
                return ParserReturnCode::InvalidCommand;
            }
        }
    }
    if (!foundEndMarker)
    {
        return ParserReturnCode::MissingEndIdentifier;
    }

    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parsePipelineBlock(uint32_t& idx)
{
    bool foundEndMarker = false;

    for (; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        // remove whitespace, newlines, etc. to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        if (line.empty() || line.find("//") != EOL)
        {
            continue;
        }

        if (line.find("##end_pipeline") != EOL)
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
    for (uint32_t idx = 0; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];

        // remove any whitespace, newlines, etc. now to stop false positives
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());

        if (line.empty() || line.find("//") != EOL)
        {
            continue;
        }

        // check for each supported command
        if (line.find("##stage:") != EOL)
        {
            // Get the shader stage type from the string e.g. ##stage: Vertex
            size_t pos = buffer[idx].find_last_of(":");
            std::string stageStr = buffer[idx].substr(pos + 1, buffer[idx++].size());

            Shader::Type stage = strToShaderType(stageStr);
            if (stage == Shader::Type::Unknown)
            {
                errorCache = ParserErrorCache {idx, ParserReturnCode::UnknownShaderType};
                return false;
            }

            ShaderDescriptor shader(stage);
            ParserReturnCode ret = parseShaderStage(idx, shader);
            if (ret != ParserReturnCode::Success)
            {
                errorCache = ParserErrorCache {idx, ret};
                return false;
            }

            descriptors.emplace_back(std::move(shader));
        }
        else if (line.find("##pipeline:") != EOL)
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

ShaderDescriptor* ShaderParser::getShaderDescriptor(const Shader::Type type)
{
    for (ShaderDescriptor& descr : descriptors)
    {
        if (descr.type == type)
        {
            return &descr;
        }
    }
    return nullptr;
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

bool ShaderParser::addStage(ShaderDescriptor& shader)
{
    // to a quick debug to make sure this stage hasn't already been added
    for (auto& descr : descriptors)
    {
        if (descr.type == shader.type)
        {
            return false;
        }
    }
    descriptors.emplace_back(shader);

    return true;
}

} // namespace VulkanAPI
