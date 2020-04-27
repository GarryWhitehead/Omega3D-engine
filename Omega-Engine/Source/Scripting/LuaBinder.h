/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "lua/lua.hpp"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

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

} // namespace OmegaEngine
