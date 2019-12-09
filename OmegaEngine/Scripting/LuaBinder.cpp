#include "LuaBinder.h"

#include "utility/Logger.h"

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

std::vector<LuaTypeRef> LuaBinder::getTable()
{
	std::vector<LuaTypeRef> tableValues;

	lua_pushnil(luaState);

	while (lua_next(luaState, -2) != 0)
	{
		LuaTypeRef ref = getGlobal();
		// just ignore invalid types
		if (ref.second.type != TypeDescriptor::Invalid)
		{
			tableValues.emplace_back(ref);
		}
		lua_pop(luaState, 1);
	}

	return tableValues;
}

LuaTypeRef LuaBinder::getGlobal()
{
	if (lua_next(luaState, -2) == 0)
	{
		return {};
	}

	int keyType = lua_type(luaState, -2);
	int valueType = lua_type(luaState, -1);

	// only strings are allowed for key types
	if (keyType != LUA_TSTRING)
	{
		lua_pop(luaState, 1);
		return {};
	}

	// get the key string
	std::string keyString = lua_tostring(luaState, -2);
	if (keyString.empty())
	{
		lua_pop(luaState, 1);
		return {};
	}

	// a fairly budget lamda to check for float or integer as all number types in lua are floats
	// this may fail due to precision errors
	auto isFloat = [](float val) { return (val != floor(val)); };

	// get the key value
	TypeDescriptor descr;
	switch (valueType)
	{
	case LUA_TNUMBER:
	{
		float value = lua_tonumber(luaState, -1);
		if (isFloat(value))
		{
			descr = TypeDescriptor{ TypeDescriptor::Float, std::to_string(value) };
		}
		else
		{
			descr = TypeDescriptor{ TypeDescriptor::Int, std::to_string(value) };
		}
		break;
	}
	case LUA_TSTRING:
	{
		std::string value = lua_tostring(luaState, -1);
		descr = TypeDescriptor{ TypeDescriptor::String, value };
		break;
	}
	case LUA_TTABLE:
	{
		LOGGER_ERROR("Nested tables are not supported at present.");
		break;
	}
	default:
		// the default type is invalid
		break;
	}

	lua_pop(luaState, 1);
	return std::make_pair(keyString, descr);
}


}    // namespace OmegaEngine