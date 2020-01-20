#include "ShaderParser.h"

#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkUtils/StringToVk.h"
#include "VulkanAPI/VkUtils/VkToString.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"


namespace VulkanAPI
{

// ================== ShaderParser =======================

ParserReturnCode ShaderParser::parseLine(const std::string type, const std::string line, std::string& output)
{
    size_t startPos = line.find(type);
    if (startPos == std::string::npos)
    {
        return ParserReturnCode::NotFound;
    }

    // find the next = ; this is done more for debugging puproses
    size_t pos = line.find_first_of('=', startPos);
    if (pos == std::string::npos)
    {
        return ParserReturnCode::InvalidTypeFormat;
    }
    std::string value = line.substr(0, pos);
    
    size_t endPos = value.find_first_of(',');
    if (endPos == std::string::npos)
    {
        return ParserReturnCode::InvalidTypeFormat;
    }
    
    output = value.substr(endPos - 1, line.size());
    output.erase(remove_if(output.begin(), output.end(), isspace), output.end());   // remove whitespace
    
    // if we are here, the shader doc hasn't been formatted correctly.
    return ParserReturnCode::Success;
}

ParserReturnCode ShaderParser::parseShaderStage(const uint32_t startIdx)
{
    // Get the shader stage type from the string e.g. ##stage: Vertex
    size_t pos = buffer[startIdx].find_last_of(":");
    std::string stageStr = buffer[startIdx].substr(pos, buffer[startIdx].size());
    stageStr.erase(remove_if(stageStr.begin(), stageStr.end(), isspace), stageStr.end());   // remove whitespace
    
    Shader::Type stage = Shader::strToShaderType(stageStr);
    if (stage == Shader::Type::Unknown)
    {
        return ParserReturnCode::UnknownShaderType;
    }
    
    auto shader = std::make_unique<ShaderDescriptor>(stage);
    bool foundEndMarker = false;
    
    for (size_t idx = startIdx; idx < buffer.size(); ++idx)
    {
        std::string& line = buffer[idx];
        if (line.find("##end_stage") != std::string::npos)
        {
            foundEndMarker = true;
            break;
        }
        
        // check for each supported stage variable type - must begin with '#'
        // input semantics - glsl code: layout (location = 0) in [TYPE] [NAME]
        if (line.find("#inputs:") != std::string::npos)
        {
            ShaderDescriptor::InOutDescriptor descr;
            ParserReturnCode ret = parseLine("Name", line, descr.name);
            ret = parseLine("Type", line, descr.type);
            
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.inputs.emplace_back(descr);
        }
        // output semantics - glsl code: layout (location = 0) out [TYPE] [NAME]
        else if (line.find("#output:") != std::string::npos)
        {
            ShaderDescriptor::InOutDescriptor descr;
            ParserReturnCode ret = parseLine("Name", line, descr.name);
            ret = parseLine("Type", line, descr.type);
            
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.inputs.emplace_back(descr);
        }
        // speciliastion constants - should be preferred to the usual #define method
        else if (line.find("#constant:") != std::string::npos)
        {
            ShaderDescriptor::ConstantDescriptor descr;
            ParserReturnCode ret = parseLine("Name", line, descr.name);
            ret = parseLine("Type", line, descr.type);
            ret = parseLine("Value", line, descr.value);
            
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            shader.constants.emplace_back(descr);
        }
        else if (line.find("#push_constant:") != std::string::npos)
        {
            ShaderDescriptor::PConstantDescriptor descr;
            ParserReturnCode ret = parseLine("Name", line, descr.name);
            
            if (ret != ParserReturnCode::Success)
            {
                return ret;
            }
            descr.type = "PushConstant";
            
            
            std::vector<std::pair<std::string, std::string>> tempBuffer;
            parseBuffer(idx + 1, tempBuffer);
            
            if (tempBuffer.empty())
            {
                return ParserReturnCode::InvalidBufferFormat;
            }
            
            shader.constants.emplace_back(descr);
        }
    }
    
    
    maxGroup = 0;
    assert(startIdx < buffer.size());
    assert(endIdx < buffer.size());

    
    
    

    // push constants - one per shader stage supported - due to the size limit that can be pushed,
    // this shouldn't be an issue
    if (shaderBlock.HasMember("PushConstants"))
    {
        const auto& constants = shaderBlock["PushConstants"].GetArray();
        for (auto& constant : constants)
        {
            if (!constant.HasMember("name"))
            {
                LOGGER_ERROR(
                    "Error while parsing block: %s. Invalid 'PushConstants' format", id.c_str());
                return false;
            }

            ShaderDescriptor::PConstantDescriptor descr;
            descr.name = constant["name"].GetString();
            descr.type = "PushConstant";

            // push constants can have a identifier, not mandatory
            if (constant.HasMember("id"))
            {
                descr.id = constant["id"].GetString();
            }

            const auto& items = constant["items"].GetArray();
            for (auto& item : items)
            {
                if (!item.HasMember("name") || !item.HasMember("type"))
                {
                    LOGGER_ERROR(
                        "Error while parsing block: %s. Invalid item format within PushConstants "
                        "block",
                        id.c_str());
                    return false;
                }

                ShaderDescriptor::Descriptor itemDescr;
                itemDescr.name = item["name"].GetString();
                itemDescr.type = item["type"].GetString();

                descr.data.emplace_back(itemDescr);
            }
            shader.pConstants.emplace_back(descr);
        }
    }

    // all samplers, ubos to import
    if (shaderBlock.HasMember("Imports"))
    {
        const auto& imports = shaderBlock["Imports"].GetArray();
        if (!imports.Empty())
        {
            for (auto& import : imports)
            {
                if (!import.HasMember("name") || !import.HasMember("type"))
                {
                    LOGGER_ERROR(
                        "Error while parsing block: %s. Invalid 'Imports' format", id.c_str());
                    return false;
                }

                std::string name = import["name"].GetString();
                std::string type = import["type"].GetString();

                // samplers and buffers can be grouped using a identifying name
                // The id will be appended to the buffer or sampler name
                uint16_t group = 0;
                if (import.HasMember("group"))
                {
                    group = std::stoi(import["group"].GetString());
                }

                if (VkUtils::isSamplerType(type))
                {
                    ShaderDescriptor::Descriptor samplerDescr;
                    samplerDescr.name = name;
                    samplerDescr.type = type;
                    samplerDescr.groupId = group;

                    std::string variant;
                    if (import.HasMember("variant"))
                    {
                        samplerDescr.variant = import["variant"].GetString();
                    }

                    shader.samplers.emplace_back(samplerDescr);
                }
                else if (VkUtils::isBufferType(type))
                {
                    // extra data for a buffer: the types and names of the buffer
                    ShaderDescriptor::BufferDescriptor buffer;
                    buffer.descr.name = name;
                    buffer.descr.type = type;
                    buffer.descr.groupId = group;

                    // optional sub id for a uniform buffer - e.g.
                    //	struct Blah
                    // {
                    // } sub_name;
                    if (import.HasMember("id"))
                    {
                        buffer.descr.id = import["id"].GetString();
                    }

                    const auto& items = import["items"].GetArray();
                    for (auto& item : items)
                    {
                        if (!item.HasMember("name") || !item.HasMember("type"))
                        {
                            LOGGER_ERROR(
                                "Error while parsing block: %s. Invalid item format within Buffer "
                                "block",
                                id.c_str());
                            return false;
                        }

                        ShaderDescriptor::Descriptor itemDescr;
                        itemDescr.name = item["name"].GetString();
                        itemDescr.type = item["type"].GetString();

                        // check whether any arrays are specified
                        if (item.HasMember("array_constant"))
                        {
                            itemDescr.arrayConst = item["array_constant"].GetString();
                        }
                        if (item.HasMember("array_value"))
                        {
                            itemDescr.arraySize = item["array_value"].GetInt();
                        }
                        buffer.data.emplace_back(itemDescr);
                    }
                    shader.ubos.emplace_back(buffer);
                }
                else
                {
                    LOGGER_ERROR(
                        "Invalid 'Imports' type specified of: %s specified for block: %s",
                        type.c_str(),
                        id.c_str());
                    return false;
                }
            }
        }
    }

    // a specific shader stage can specify a code path; this allows code to be reused amoungst
    // numerous shaders
    if (shaderBlock.HasMember("CodePath"))
    {
        shader.codePath = shaderBlock["CodePath"].GetString();
        if (shader.codePath.empty())
        {
            LOGGER_ERROR("Shader code path specified but no filename found.");
            return false;
        }
    }

    return true;
}

void ShaderParser::addStage(ShaderDescriptor* shader)
{
    descriptors.emplace_back(shader);
}

void ShaderParser::parseRenderBlock(rapidjson::Document& doc)
{
    // parse all the raster state data from the file :
    // Note: rather than storing this data in a raw string fromat and then dealing with it later in
    // the compiler, the render state data is extracted in it's final format and passed to the
    // compiler later
    if (doc.HasMember("DepthStencilState"))
    {
        const rapidjson::Value& depth = doc["DepthStencilState"];
        if (depth.HasMember("DepthTestEnable"))
        {
            renderState->dsState.testEnable = depth["DepthTestEnable"].GetBool();
        }
        if (depth.HasMember("DepthWriteEnable"))
        {
            renderState->dsState.writeEnable = depth["DepthWriteEnable"].GetBool();
        }
        if (depth.HasMember("CompareOp"))
        {
            renderState->dsState.compareOp =
                VkUtils::vkCompareOpFromString(depth["CompareOp"].GetString());
        }
    }

    if (doc.HasMember("RasterState"))
    {
        const rapidjson::Value& rState = doc["RasterState"];
        if (rState.HasMember("PolygonMode"))
        {
            renderState->rastState.polygonMode =
                VkUtils::vkPolygonFromString(rState["PolygonMode"].GetString());
        }
        if (rState.HasMember("CullMode"))
        {
            renderState->rastState.cullMode =
                VkUtils::vkCullModeFromString(rState["CullMode"].GetString());
        }
        if (rState.HasMember("FrontFace"))
        {
            renderState->rastState.frontFace =
                VkUtils::vkFrontFaceFromString(rState["FrontFace"].GetString());
        }
    }

    if (doc.HasMember("Sampler"))
    {
        const rapidjson::Value& sampler = doc["Sampler"];
        if (sampler.HasMember("MagFilter"))
        {
            renderState->sampler.magFilter =
                VkUtils::vkFilterToString(sampler["MagFilter"].GetString());
        }
        if (sampler.HasMember("MinFilter"))
        {
            renderState->sampler.minFilter =
                VkUtils::vkFilterToString(sampler["MinFilter"].GetString());
        }
        if (sampler.HasMember("AddressModeU"))
        {
            renderState->sampler.addrModeU =
                VkUtils::vkAddressModeToString(sampler["AddressModeU"].GetString());
        }
        if (sampler.HasMember("AddressModeV"))
        {
            renderState->sampler.addrModeV =
                VkUtils::vkAddressModeToString(sampler["AddressModeV"].GetString());
        }
        if (sampler.HasMember("AddressModeW"))
        {
            renderState->sampler.addrModeW =
                VkUtils::vkAddressModeToString(sampler["AddressModeW"].GetString());
        }
    }

    // This isn't mandatory, individual stages can specify there own paths to shader code
    if (doc.HasMember("CodePath"))
    {
        codePath = doc["CodePath"].GetString();
        if (codePath.empty())
        {
            LOGGER_WARN("Code path specified but is empty.");
        }
    }
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
           
        }
        ++idx;
    }
    
    parseRenderBlock(doc);

    // If a compute shdaer life is a bit easier, otherwise check for each shader stage and extract
    // all information into a raw format for the compiler to use
    ParserReturnCode ret = findStage(Shader::Type::Compute, startIdx, endIdx);
    if (ret == ParserReturnCode::Success)
    {
        auto compShader = std::make_unique<ShaderDescriptor>(Shader::Type::Compute);
        if (!readShader(doc, *compShader, "ComputeShader", groupSize))
        {
            return false;
        }
        descriptors.emplace_back(std::move(compShader));
        return true;
    }
    else if (ret != ParserReturnCode::NotFound)
    {
        // print error here!
        return false;
    }
    
    for (uint8_t i = 0; i < Shader::Type::Count; ++i)
    {
        ret = findStage(static_cast<Shader::Type>(i), startIdx, endIdx);
        if (ret == ParserReturnCode::Success)
        {
            auto shader = std::make_unique<ShaderDescriptor>(static_cast<Shader::Type>(i));
            if (!parseShaderStage(*shader, "VertexShader", groupSize, startIdx, endIdx))
            {
                return false;
            }
            descriptors.emplace_back(std::move(vertShader));
        }
        else if (ret != ParserReturnCode::NotFound)
        {
            // print error here!
            return false;
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

    // create the render block
    renderState = new RenderStateBlock();
    assert(renderState);

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

    rapidjson::Document doc;

    if (doc.Parse(buffer.c_str()).HasParseError())
    {
        LOGGER_ERROR("Error whilst trying to parse shader json file.");
        return false;
    }

    Util::String vertexId = Shader::shaderTypeToString(type);
    uint16_t maxGroup = 0;

    if (!readShader(doc, *shader, vertexId, maxGroup))
    {
        LOGGER_ERROR(
            "Unable to read shader block from json file; filename = %s.", filename.c_str());
        return false;
    }

    return true;
}

} // namespace VulkanAPI
