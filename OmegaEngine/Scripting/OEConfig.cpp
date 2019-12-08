#include "OEConfig.h"

#include <cassert>

namespace OmegaEngine
{

EngineConfig::EngineConfig(lua_State* state)
    : luaState(state)
{
}

bool EngineConfig::init()
{
	assert(luaState);
	if (luaL_loadfile(luaState, config_filename.c_str) || lua_pcall(luaState, 0, 0, 0))
	{
		// if no file found, we will use the default settings, and save this to disc
	}

	auto t = lua_getglobal(luaState, "window");

}


}    // namespace OmegaEngine
