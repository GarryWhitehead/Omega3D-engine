#pragma once

#include <assert.h>

namespace OEMaths
{
	
	// some popular maths conversions (haven't decided were to locate these yet!)
	template <typename T>
	T radians(const T deg)
	{
		return deg * M_PI / 180;
	}
}

