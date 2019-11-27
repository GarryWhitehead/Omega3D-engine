#pragma once

#include "OEMaths/OEMaths_VecN.h"

namespace OEMaths
{

template <typename T>
class VecN<T, 3> : public MathOperators<VecN, T, 3>
{
public:
	VecN() :
		x(T(0)),
		y(T(0)),
		z(T(0))
	{
	}

	VecN(const T& n)
	    : x(n)
	    , y(n)
	    , z(n)
	{
	}

	VecN(const T& in_x, const T& in_y, const T& in_z)
	    : x(in_x)
	    , y(in_y)
	    , z(in_z)
	{
	}

	VecN(VecN<T, 2>& vec, const T& value)
	{
		VecN{ vec.data[0], vec.data[1], value };
	}
    
    /**
     * Only makes sense to cross a vector3, hence why this function is here
     */
    static constexpr VecN<T, 3> cross(VecN<T, 3>& vec1, VecN<T, 3>& vec2)
    {
        VecN<T, 3> result;
        result.x = vec1.y * vec2.z - vec1.z * vec2.y;
        result.y = vec1.z * vec2.x - vec1.x * vec2.z;
        result.z = vec1.x * vec2.y - vec1.y * vec2.x;
        return result;
    }

public:
	union
	{
		T data[3];

		VecN<T, 2> xy;
		VecN<T, 2> st;
		VecN<T, 2> rg;

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

using vec3f = VecN<float, 3>;
using vec3d = VecN<double, 3>;
using vec3u16 = VecN<uint16_t, 3>;
using vec3u32 = VecN<uint32_t, 3>;
using vec3u64 = VecN<uint64_t, 3>;
using vec3i16 = VecN<int16_t, 3>;
using vec3i32 = VecN<int32_t, 3>;
using vec3i64 = VecN<int64_t, 3>;

// colour storage
using colour3 = VecN<float, 3>;

}    // namespace OEMaths
