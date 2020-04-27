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
