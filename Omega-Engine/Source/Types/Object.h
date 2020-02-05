#pragma once

#include "omega-engine/Object.h"

#include <cstdint>
#include <cassert>

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