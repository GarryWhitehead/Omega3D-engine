#pragma once

#include <assert.h>
#include <cstdint>
#include <stdlib.h>

namespace Util
{
	// function to generate a unique id for any given type.
	// Needs making thread safe
	template <typename T>
	class unique_id
	{
		static char type_id;

	public:
		static uintptr_t getId() 
		{
			return reinterpret_cast<uintptr_t>(&type_id);
		}
	};

	template <typename T>
	char unique_id<T>::type_id;

	// used for returning id of structs and classes
	template<typename T>
	inline char event_type_id()
	{
		std::unique_ptr<T> type;
		return Util::unique_id<decltype(type)>::getId();
	}

	// aligned memory allocation
	void* alloc_align(uint32_t alignment_size, uint32_t size);

}
