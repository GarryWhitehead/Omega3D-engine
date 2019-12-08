#pragma once

#include "Scripting/LuaBinder.h"

#include "utility/String.h"

#include "lua/lua.hpp"

#include <vector>

namespace OmegaEngine
{

class EngineConfig
{
public:


	const Util::String config_filename = "engine_config.ini";

	EngineConfig() = delete;
	EngineConfig(lua_State* state);

	bool init();

private:
	lua_State* luaState = nullptr;
};

}    // namespace OmegaEngine