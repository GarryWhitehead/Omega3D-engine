#pragma once

#include <stdlib.h>
#include <mm_malloc.h>

namespace Util
{

class AlignedAlloc
{

public:
    
    AlignedAlloc() = delete;

	AlignedAlloc(size_t size, size_t alignment)
	{
#if defined(_MSC_VER) || defined(__MINGW32__)
		data = _aligned_malloc(size, alignment);
#else
		if (!posix_memalign(&data, alignment, size))
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

}    // namespace Util
