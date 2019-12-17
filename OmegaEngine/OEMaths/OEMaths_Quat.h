#pragma once

#include "OEMaths_VecN.h"
#include "OEMaths_Vec3.h"

#include <cassert>
#include <cmath>

namespace OEMaths
{

template <typename T, size_t size = 4>
class Quarternion : public MathOperators<Quarternion, T, size>
{
public:
	Quarternion()
	    : x(T(0))
	    , y(T(0))
	    , z(T(0))
	    , w(T(0))
	{
	}

	Quarternion(const T& n)
	    : x(n)
	    , y(n)
	    , z(n)
	    , w(n)
	{
	}

	Quarternion(const T& in_x, const T& in_y, const T& in_z, const T& in_w)
	    : x(in_x)
	    , y(in_y)
	    , z(in_z)
	    , w(in_w)
	{
	}

	Quarternion(VecN<T, 3>& vec, const T& value)
	{
		VecN{ vec.data[0], vec.data[1], vec.data[3], value };
	}

	T& operator[](const size_t idx)
	{
		assert(idx < 4);
		return data[idx];
	}

	// creates a quaternion from a 3-d axis and angle
	void setFromAxisAngle(const VecN<T, 3>& axis, T angle)
	{
		Quarternion{ std::sin(angle * T(0.5)) * normalise(axis), std::cos(angle * T(0.5)) };
	}

	/**
	* Extracts a quaternin from a matrix type. This code is identical to the code in Eigen.
	*/
    ::OEMaths::Quarternion<T, size> matToQuat(MatN<T, 4, 4>& mat)
	{
        ::OEMaths::Quarternion<T, size> quat;

		// Compute the trace to see if it is positive or not.
		const T trace = mat[0][0] + mat[1][1] + mat[2][2];

		// check the sign of the trace
		if (trace > 0)
		{
			// trace is positive
			T s = std::sqrt(trace + 1);
			quat.w = T(0.5) * s;
			s = T(0.5) / s;
			quat.x = (mat[1][2] - mat[2][1]) * s;
			quat.y = (mat[2][0] - mat[0][2]) * s;
			quat.z = (mat[0][1] - mat[1][0]) * s;
		}
		else
		{
			// trace is negative
			// Find the index of the greatest diagonal
			size_t i = 0;
			if (mat[1][1] > mat[0][0])
			{
				i = 1;
			}
			if (mat[2][2] > mat[i][i])
			{
				i = 2;
			}

			// Get the next indices: (n+1)%3
			static constexpr size_t next_ijk[3] = { 1, 2, 0 };
			size_t j = next_ijk[i];
			size_t k = next_ijk[j];
			T s = std::sqrt((mat[i][i] - (mat[j][j] + mat[k][k])) + 1);
			quat[i] = T(0.5) * s;
			if (s != 0)
			{
				s = T(0.5) / s;
			}
			quat.w = (mat[j][k] - mat[k][j]) * s;
			quat[j] = (mat[i][j] + mat[j][i]) * s;
			quat[k] = (mat[i][k] + mat[k][i]) * s;
		}
		return quat;
	}


public:
	union {
		T data[4];

		VecN<T, 3> xyz;
		VecN<T, 2> xy;

		struct
		{
			T x;
			T y;
			T z;
			T w;
		};
	};
};

using quatf = Quarternion<float>;
using quatd = Quarternion<double>;

// Here for imlementation at some point
/*quatf quatf::cubic_mix(quatf& q1, quatf& q2, quatf& q3, float u)
{
	quatf result;
	float a0, a1, a2, a3, u_sqr;
	u_sqr = u * u;

	// x
	a0 = -0.5f * this->x + 1.5f * q1.x - 1.5f * q2.x + 0.5f * q3.x;
	a1 = this->x - 2.5f * q1.x + 2.0f * q2.x - 0.5f * q3.x;
	a2 = -0.5f * this->x + 0.5f * q2.x;
	a3 = q1.x;
	result.x = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;

	// y
	a0 = -0.5f * this->y + 1.5f * q1.y - 1.5f * q2.y + 0.5f * q3.y;
	a1 = this->y - 2.5f * q1.y + 2.0f * q2.y - 0.5f * q3.y;
	a2 = -0.5f * this->y + 0.5f * q2.y;
	a3 = q1.y;
	result.y = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;

	// z
	a0 = -0.5f * this->z + 1.5f * q1.z - 1.5f * q2.z + 0.5f * q3.z;
	a1 = this->z - 2.5f * q1.z + 2.0f * q2.z - 0.5f * q3.z;
	a2 = -0.5f * this->z + 0.5f * q2.z;
	a3 = q1.z;
	result.z = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;

	// w
	a0 = -0.5f * this->w + 1.5f * q1.w - 1.5f * q2.w + 0.5f * q3.w;
	a1 = this->w - 2.5f * q1.w + 2.0f * q2.w - 0.5f * q3.w;
	a2 = -0.5f * this->w + 0.5f * q2.w;
	a3 = q1.w;
	result.w = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;
	return result;
}*/

}    // namespace OEMaths
