#pragma once

#include "OEMaths/OEMaths_VecN.h"

#include <cstdint>

namespace OEMaths
{

template <typename T>
class vecN<T, 2> : public MathOperators<vecN, T, 2>
{
public:
	vecN()
	    : x(T(0))
	    , y(T(0))
	{
	}

	vecN(const T& n)
	    : x(n)
	    , y(n)
	{
	}

	vecN(const T& in_x, const T& in_y)
	    : x(in_x)
	    , y(in_y)
	{
	}


public:
	union
	{
		T data[2];

		struct
		{
			T x;
			T y;
		};

		struct
		{
			T r;
			T g;
		};
	};
};

using vec2f = vecN<float, 2>;
using vec2d = vecN<double, 2>;
using vec2u16 = vecN<uint16_t, 2>;
using vec2u32 = vecN<uint32_t, 2>;
using vec2u64 = vecN<uint64_t, 2>;
using vec2i16 = vecN<int16_t, 2>;
using vec2i32 = vecN<int32_t, 2>;
using vec2i64 = vecN<int64_t, 2>;

}    // namespace OEMaths
