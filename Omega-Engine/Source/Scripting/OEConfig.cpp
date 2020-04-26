#include "OEConfig.h"

#include "utility/Logger.h"

#include <cassert>

namespace OmegaEngine
{

void EngineConfig::clear()
{
	configs.clear();
}

bool EngineConfig::hasConfig(std::string name) const
{
	auto iter = configs.find(name);
	if (iter == configs.end())
	{
		return false;
	}
	return true;
}

TypeDescriptor::Type EngineConfig::getType(std::string name) const
{
	auto iter = configs.find(name);
	if (iter == configs.end())
	{
		return TypeDescriptor::Invalid;
	}
	return iter->second.type;
}

bool EngineConfig::empty() const
{
	return configs.empty();
}

std::vector<float> EngineConfig::buildVec(TypeDescriptor& descr, uint8_t vecSize)
{    
    std::string vecStr = descr.value;
    std::vector<float> values;
    
    size_t pos = vecStr.find_first_of(',');
    while (pos != std::string::npos)
    {
        values.emplace_back(std::stof(vecStr.substr(pos - 1, vecStr.size())));
        vecStr = vecStr.substr(pos + 1, vecStr.size());
        pos = vecStr.find_first_of(',');
    }
    values.emplace_back(std::stof(vecStr));
    
    if (values.empty() || values.size() < 2)
    {
        return {};
    }
    
    size_t arraySize = values.size();
    if (arraySize != vecSize)
    {
        LOGGER_ERROR("Mismatch between configuration data size of %zu and specified vector size of %i.", arraySize, vecSize);
    }
    
    return values;
}

void EngineConfig::insertVec(std::string name, const float values[], uint32_t size)
{
    std::string vecString;
    for (uint32_t i = 0; i < size - 1; ++i)
    {
        vecString += std::to_string(values[i]) + ',';
    }
    vecString += std::to_string(values[size - 1]);
    configs.emplace(name, TypeDescriptor{ LuaBinder::arraySizeToVec(size), vecString });
}

OEMaths::vec2f EngineConfig::findOrInsertVec2(std::string name, const float defaultVec[])
{
    uint32_t size = sizeof(*defaultVec) / sizeof(float);
    assert(size == 2);
    
    auto iter = configs.find(name);
    if (iter == configs.end())
    {
        insertVec(name, defaultVec, size);
        return OEMaths::vec2f{ defaultVec[0], defaultVec[1] };
    }
    
    std::vector<float> values = buildVec(iter->second, 2);
    assert(!values.empty());
    
    return OEMaths::vec2f{ values[0], values[1] };
}

OEMaths::vec3f EngineConfig::findOrInsertVec3(std::string name, const float defaultVec[])
{
    uint32_t size = sizeof(*defaultVec) / sizeof(float);
    assert(size == 3);
    
    auto iter = configs.find(name);
    if (iter == configs.end())
    {
        insertVec(name, defaultVec, size);
        return OEMaths::vec3f{ defaultVec[0], defaultVec[1], defaultVec[2] };
    }
    
    std::vector<float> values = buildVec(iter->second, 3);
    assert(!values.empty());
    
    return OEMaths::vec3f{ values[0], values[1], values[2] };
}

OEMaths::vec4f EngineConfig::findOrInsertVec4(std::string name, const float defaultVec[])
{
    uint32_t size = sizeof(*defaultVec);
    assert(size == 4);
    
    auto iter = configs.find(name);
    if (iter == configs.end())
    {
        insertVec(name, defaultVec, size);
        return OEMaths::vec4f{ defaultVec[0], defaultVec[1], defaultVec[2], defaultVec[3] };
    }
    
    std::vector<float> values = buildVec(iter->second, 4);
    assert(!values.empty());
    
    return OEMaths::vec4f{ values[0], values[1], values[2], values[3] };
}

// =======================================================================================

ConfigParser::ConfigParser(LuaBinder& binder, EngineConfig& config)
    : binder(binder)
    , config(config)
{
}

bool ConfigParser::parse(Util::String filename)
{
	lua_State* luaState = binder.getState();
	if (luaL_loadfile(luaState, filename.c_str()) || lua_pcall(luaState, 0, 0, 0))
	{
		LOGGER_WARN("No configuration file found. Using default settings.");
	}

	lua_pushglobaltable(luaState);
	lua_pushnil(luaState);

	// get all global variables from script
	while (lua_next(luaState, -2))
	{
		if (lua_istable(luaState, -1))
		{
			// check whether this is an array - this will be converted to a vector
            if (binder.isArray())
            {
                LuaTypeRef ref = binder.getArray();
                if (ref.second.type != TypeDescriptor::Invalid)
                {
                    config.configs.insert(ref);
                }
            }
            
            // if not an array, must be a table
            auto tableValues = binder.getTable();
			for (auto& val : tableValues)
			{
				// this is copy insertion though not a performance related function so its ok
				config.configs.insert(val);
			}
		}
		else if (lua_isnumber(luaState, -1))
		{
			LuaTypeRef ref = binder.getGlobal();
			if (ref.second.type != TypeDescriptor::Invalid)
			{
				config.configs.insert(ref);
			}
		}
		else
		{
			LOGGER_WARN("Invalid config type detected with id %s", lua_tostring(luaState, -2));
		}
	}
    
    return true;
}


}    // namespace OmegaEngine
