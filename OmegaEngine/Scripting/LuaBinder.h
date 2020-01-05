#pragma once

#include "lua/lua.hpp"

#include <string>
#include <utility>
#include <vector>
#include <cassert>

namespace OmegaEngine
{

struct TypeDescriptor
{
	enum Type
	{
		Int,
		Float,
		String,
        Vec2,
        Vec3,
        Vec4,
		Invalid
	};

	Type type = Type::Invalid;
	std::string value;
};

using LuaTypeRef = std::pair<std::string, TypeDescriptor>;

class LuaBinder
{
public:
	LuaBinder() = default;

	void init();
	void destroy();

    bool isArray();
    LuaTypeRef getArray();
    
	LuaTypeRef getGlobal();

	std::vector<LuaTypeRef> getTable();
    
    static TypeDescriptor::Type arraySizeToVec(uint8_t size);
    
	lua_State* getState()
	{
		assert(luaState);
		return luaState;
	}

private:
	lua_State* luaState = nullptr;
};

}    // namespace OmegaEngine
