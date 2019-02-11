#include "GeneralUtil.h"


namespace Util
{
	static void* alloc_align(uint32_t alignment_size, uint32_t size)
	{
		void *data = nullptr;
		data = _aligned_malloc(alignment_size, size);
		assert(data);
		return data;
	}
}
