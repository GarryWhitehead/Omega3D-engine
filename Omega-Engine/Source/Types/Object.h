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

#include "omega-engine/Object.h"

#include <cassert>
#include <cstdint>

namespace OmegaEngine
{

class ObjectHandle
{
public:
    ObjectHandle() = delete;
    explicit ObjectHandle(uint64_t h) : handle(h)
    {
    }

    bool operator==(const ObjectHandle rhs)
    {
        return handle == rhs.handle;
    }

    uint64_t get() const
    {
        assert(handle != UINT64_MAX);
        return handle;
    }

    void invalidate()
    {
        handle = UINT64_MAX;
    }

    bool valid() const
    {
        return handle != UINT64_MAX;
    }

private:
    uint64_t handle;
};

class OEObject : public Object
{

public:
    OEObject(const uint64_t id);

    // operator overloads
    bool operator==(const OEObject& obj) const;

    // helper functions
    uint64_t getId() const;
    void setId(const uint64_t objId);
    bool isActive() const;

private:
    uint64_t id;
    bool active = true;
};

} // namespace OmegaEngine
