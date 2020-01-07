
#include "GltfExtension.h"

namespace OmegaEngine
{

OEMaths::vec3f GltfExtension::tokenToVec3(Util::String str)
{
    OEMaths::vec3f output;
    auto split = Util::String::split(str, ' ');
    assert(split.size() == 3);
    return OEMaths::vec3f(split[0].toFloat(), split[1].toFloat(), split[2].toFloat());
}

Util::String GltfExtension::find(Util::String ext)
{
    auto iter = extensions.find(ext.c_str());
    if (iter == extensions.end())
    {
        return "";
    }
    return iter->second;
}

bool GltfExtension::prepare(const cgltf_extras& extras, cgltf_data& data)
{
    // first check whther there are any extensions
    cgltf_size extSize;
    cgltf_copy_extras_json(&data, &extras, nullptr, &extSize);
    if (!extSize)
    {
        return false;
    }

    // alloc mem for the json extension data
    char* jsonData = new char[extSize + 1];

    cgltf_result res = cgltf_copy_extras_json(&data, &extras, jsonData, &extSize);
    if (!res)
    {
        LOGGER_ERROR("Unable to prepare extension data. Error code: %d\n", res);
        return false;
    }

    jsonData[extSize] = '\0';

    // create json file from data blob - using cgltf implementation here
    jsmn_parser jsonParser;
    jsmn_init(&jsonParser);
    const int tokenCount = jsmn_parse(&jsonParser, jsonData, extSize, nullptr, 0);
    if (tokenCount < 1)
    {
        LOGGER_ERROR("Unable to parse extensions json file.\n");
        return false;
    }

    // we know the token size, so allocate memory for this and get the tokens
    std::vector<jsmntok_t> tokenData;
    jsmn_parse(&jsonParser, jsonData, extSize, tokenData.data(), tokenCount);

    // now convert everything to a string for easier storage
    std::vector<Util::String> temp;
    for (const jsmntok_t& token : tokenData)
    {
        Util::String startStr(Util::String::valueToString(&jsonData[token.start]));
        Util::String endStr(Util::String::valueToString(token.end - token.start));
        temp.emplace_back(startStr);
        temp.emplace_back(endStr);
    }

    // should be divisible by 2 otherwise we will overflow when creating the output
    if ((temp.size() % 2) != 0)
    {
        LOGGER_ERROR("Error while parsing tokens. Invalid size.\n");
        return false;
    }

    // create the output - <extension name, value>
    for (size_t i = 0; i < temp.size(); i += 2)
    {
        extensions.emplace(temp[i].c_str(), temp[i + 1]);
    }

    return true;
}

}
