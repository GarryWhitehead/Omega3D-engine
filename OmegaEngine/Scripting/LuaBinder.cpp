#include "LuaBinder.h"

namespace OmegaEngine
{

void LuaBinder::init()
{
	luaState = luaL_newstate();
	luaL_openlibs(luaState);
}

void LuaBinder::destroy()
{
	lua_close(luaState);
	luaState = nullptr;
}

}    // namespace OmegaEngine