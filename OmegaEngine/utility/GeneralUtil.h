#pragma once

#include <assert.h>
#include <cstdint>
#include <stdlib.h>

// stop the warning about conversion from uintptr to char
#pragma warning(disable: 4244)

namespace Util
{
	// function to generate a unique id for any given type.
	// Uses crc-32c to  
	uint32_t generateTypeId(const char* typeName);

	template <typename T>
	class TypeId
	{
	public:

		static uint32_t id()
		{
#if defined(_MSC_VER)
			return generateTypeId(__FUNCTION__);
#elif defined(__GNUC__) 
			return generateTypeId(__PRETTY_FUNCTION__);
#else
			return generateTypeId(typeid(T).name());
#endif
		}
	};

	// aligned memory allocation
	void* alloc_align(size_t alignmentSize, size_t size);

}
