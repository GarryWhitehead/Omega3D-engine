#pragma once

#include "lua/lua.hpp"

namespace OmegaEngine
{

class LuaBinder
{
public:

	LuaBinder() = default;

	void init();
	void destroy();

private:
	
	lua_State* luaState = nullptr;

};

}    // namespace OmegaEngine