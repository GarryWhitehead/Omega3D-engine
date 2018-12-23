#pragma once

#include <cstdint>

namespace Util
{
	// claculates the min and max of a  1d  represented grid by converting to a 2d grid. Also returns adjusted width adn height based upon the total 2d matrix size
	// totalSize must be a n * n grid. The gridSize must also be a n * n grid and also be divisibale by 2. Hence grid sizes of n - 1 were n are 3, 5, 7, 9, 11.....
	// the grid is represented as a sequential set of numbers and the start specifies a number within this set.
	bool min_max_1dto2dgrid(uint32_t start, uint32_t totalSize, uint32_t gridSize, uint32_t& outWidth, uint32_t& outHeight, uint32_t& outMin, uint32_t& outMax);

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
		return Util::unique_id<decltype(T)>::getId();
	}
}
