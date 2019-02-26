#include "GeneralUtil.h"


namespace Util
{
	void* alloc_align(uint32_t alignment_size, uint32_t size)
	{
		void *data = nullptr;
		data = _aligned_malloc(size, alignment_size);
		assert(data);
		return data;
	}
}
