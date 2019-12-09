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

ConfigParser::ConfigParser(LuaBinder& binder, EngineConfig& config)
    : binder(binder)
    , config(config)
{
}

bool ConfigParser::parse(Util::String filename)
{
	lua_State* luaState = binder.getState();
	if (luaL_loadfile(luaState, filename.c_str) || lua_pcall(luaState, 0, 0, 0))
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
}

}    // namespace OmegaEngine
