#pragma once

#include <cstdint>
#include <cassert>

namespace OmegaEngine
{

class VkHandle
{
public:
    VkHandle() = default;
    explicit VkHandle(const uint64_t handle) : handle(handle)
    {
    }

    uint64_t get() const
    {
        assert(handle != UINT64_MAX);
        return handle;
    }

private:
    uint64_t handle = UINT64_MAX;
};

using FBufferHandle = VkHandle;
using RPassHandle = VkHandle;

} // namespace OmegaEngine
