#pragma once

#include "OEMaths_VecN.h"

#include <cstdint>

namespace OEMaths
{

template <typename T>
class VecN<T, 2> : public MathOperators<VecN, T, 2>
{
public:
	VecN()
	    : x(T(0))
	    , y(T(0))
	{
	}

	VecN(const T& n)
	    : x(n)
	    , y(n)
	{
	}

	VecN(const T& in_x, const T& in_y)
	    : x(in_x)
	    , y(in_y)
	{
	}

    inline constexpr T& operator[](const size_t idx)
    {
        assert(idx < 2);
        return data[idx];
    }
    
    inline constexpr T operator[](const size_t idx) const
    {
        assert(idx < 2);
        return data[idx];
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

using vec2f = VecN<float, 2>;
using vec2d = VecN<double, 2>;
using vec2u16 = VecN<uint16_t, 2>;
using vec2u32 = VecN<uint32_t, 2>;
using vec2u64 = VecN<uint64_t, 2>;
using vec2i16 = VecN<int16_t, 2>;
using vec2i32 = VecN<int32_t, 2>;
using vec2i64 = VecN<int64_t, 2>;

}    // namespace OEMaths
