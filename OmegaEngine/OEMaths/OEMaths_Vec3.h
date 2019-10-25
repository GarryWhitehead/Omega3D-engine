#pragma once

#include "OEMaths/OEMaths_VecN.h"

namespace OEMaths
{

template <typename T>
class vecN<T, 3> : public MathOperators<vecN, T, 3>
{
public:
	vecN() :
		x(T(0)),
		y(T(0)),
		z(T(0))
	{
	}

	vecN(const T& n)
	    : x(n)
	    , y(n)
	    , z(n)
	{
	}

	vecN(const T& in_x, const T& in_y, const T& in_z)
	    : x(in_x)
	    , y(in_y)
	    , z(in_z)
	{
	}

	vecN(vecN<T, 2>& vec, const T& value)
	{
		vecN{ vec.data[0], vec.data[1], value };
	}

	

public:
	union
	{
		T data[3];

		vecN<T, 2> xy;
		vecN<T, 2> st;
		vecN<T, 2> rg;

		struct
		{
			T x;
			T y;
			T z;
		};

		struct
		{
			T r;
			T g;
			T b;
		};
	};
};

using vec3f = vecN<float, 3>;
using vec3d = vecN<double, 3>;
using vec3u16 = vecN<uint16_t, 3>;
using vec3u32 = vecN<uint32_t, 3>;
using vec3u64 = vecN<uint64_t, 3>;
using vec3i16 = vecN<int16_t, 3>;
using vec3i32 = vecN<int32_t, 3>;
using vec3i64 = vecN<int64_t, 3>;

}    // namespace OEMaths
