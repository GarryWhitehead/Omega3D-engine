#pragma once

#include <assert.h>

#define M_PI 3.141592653589793238462643383279502884197169399375

namespace OEMaths
{
	
	// some popular maths conversions (haven't decided were to locate these yet!)
	template <typename T>
	T radians(const T deg)
	{
		return deg * M_PI / 180;
	}
}

