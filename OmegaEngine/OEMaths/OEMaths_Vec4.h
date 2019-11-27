#pragma once

#include "OEMaths/OEMaths_VecN.h"

#include <cstdint>

namespace OEMaths
{
template <typename T>
class VecN<T, 4> : public MathOperators<VecN, T, 4>
{
public:
	VecN()
	    : x(T(0))
	    , y(T(0))
	    , z(T(0))
	    , w(T(0))
	{
	}

	VecN(const T& n)
	    : x(n)
	    , y(n)
	    , z(n)
	    , w(n)
	{
	}

	VecN(const T& in_x, const T& in_y, const T& in_z, const T& in_w)
	    : x(in_x)
	    , y(in_y)
	    , z(in_z)
	    , w(in_w)
	{
	}

	VecN(VecN<T, 3>& vec, const T& value)
	{
		VecN{ vec.data[0], vec.data[1], vec.data[3], value };
	}

	T& operator[](const size_t idx) 
	{
		assert(idx < 4);
		return data[idx];
	}

public:
	union {
		T data[4];

		VecN<T, 3> xyz;
		VecN<T, 3> stu;
		VecN<T, 3> rgb;
		VecN<T, 2> xy;
		VecN<T, 2> st;
		VecN<T, 2> rg;

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

using vec4f = VecN<float, 4>;
using vec4d = VecN<double, 4>;
using vec4u16 = VecN<uint16_t, 4>;
using vec4u32 = VecN<uint32_t, 4>;
using vec4u64 = VecN<uint64_t, 4>;
using vec4i16 = VecN<int16_t, 4>;
using vec4i32 = VecN<int32_t, 4>;
using vec4i64 = VecN<int64_t, 4>;

// colour storage
using colour4 = VecN<float, 4>;

}    // namespace OEMaths
