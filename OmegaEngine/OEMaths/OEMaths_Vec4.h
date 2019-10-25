#pragma once

#include "OEMaths/OEMaths_VecN.h"

#include <cstdint>

namespace OEMaths
{
template <typename T>
class vecN<T, 4> : public MathOperators<vecN, T, 4>
{
public:
	vecN()
	    : x(T(0))
	    , y(T(0))
	    , z(T(0))
	    , w(T(0))
	{
	}

	vecN(const T& n)
	    : x(n)
	    , y(n)
	    , z(n)
	    , w(n)
	{
	}

	vecN(const T& in_x, const T& in_y, const T& in_z, const T& in_w)
	    : x(in_x)
	    , y(in_y)
	    , z(in_z)
	    , w(in_w)
	{
	}

	vecN(vecN<T, 3>& vec, const T& value)
	{
		vecN{ vec.data[0], vec.data[1], vec.data[3], value };
	}

	T& operator[](const size_t idx) 
	{
		assert(idx < 4);
		return data[idx];
	}

public:
	union {
		T data[4];

		vecN<T, 3> xyz;
		vecN<T, 3> stu;
		vecN<T, 3> rgb;
		vecN<T, 2> xy;
		vecN<T, 2> st;
		vecN<T, 2> rg;

		struct
		{
			T x;
			T y;
			T z;
			T w;
		};

		struct
		{
			T r;
			T g;
			T b;
			T a;
		};
	};
};

using vec4f = vecN<float, 4>;
using vec4d = vecN<double, 4>;
using vec4u16 = vecN<uint16_t, 4>;
using vec4u32 = vecN<uint32_t, 4>;
using vec4u64 = vecN<uint64_t, 4>;
using vec4i16 = vecN<int16_t, 4>;
using vec4i32 = vecN<int32_t, 4>;
using vec4i64 = vecN<int64_t, 4>;

}    // namespace OEMaths