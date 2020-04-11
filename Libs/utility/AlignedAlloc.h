#pragma once
#include <cassert>
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <stdlib.h>
#else
#include <mm_malloc.h>
#endif

namespace Util
{

class AlignedAlloc
{

public:
    AlignedAlloc() = default;

    void alloc(size_t size, size_t alignment)
    {
        assert(size > 0);

#if defined(_MSC_VER) || defined(__MINGW32__)
        data = _aligned_malloc(size, alignment);
#else
        if (posix_memalign((void**)&data, alignment, size) != 0)
        {
            data = nullptr;
        }
#endif
    }

    ~AlignedAlloc()
    {
#if defined(_MSC_VER) || defined(__MINGW32__)
        _aligned_free(data);
#else
        free(data);
#endif
    }

    AlignedAlloc(const AlignedAlloc&) = delete;
    AlignedAlloc& operator=(const AlignedAlloc&) = delete;


    void* getData()
    {
        return data;
    }

    bool empty()
    {
        return !data;
    }

private:
    void* data = nullptr;
};

} // namespace Util
