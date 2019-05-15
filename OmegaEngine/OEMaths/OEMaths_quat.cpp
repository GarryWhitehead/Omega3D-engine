#include "OEMaths_quat.h"
#include "OEMaths.h"
#include <cassert>

namespace OEMaths
{

	// conversion functions
	void quatf::convert_F(const float* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;

		this->x = *ptr;
		++ptr;
		this->y = *ptr;
		++ptr;
		this->z = *ptr;
		++ptr;
		this->w = *ptr;
	}

	void quatf::convert_D(const double* data)
	{
		assert(data != nullptr);
		double* ptr = (double*)data;

		this->x = (float)*ptr;
		++ptr;
		this->y = (float)*ptr;
		++ptr;
		this->z = (float)*ptr;
		++ptr;
		this->w = (float)*ptr;
	}

	// conversion
	
	float quatf::length()
	{
		return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
	}

	void quatf::normalise()
	{
		float l = length();

		this->x = this->x / l;
		this->y = this->y / l;
		this->z = this->z / l;
		this->w = this->w / l;
	}

	quatf quatf::linear_mix(quatf& q1, float u)
	{
		quatf result;
		result.x = this->x * (1.0f - u) + q1.x * u;
		result.y = this->y * (1.0f - u) + q1.y * u;
		result.z = this->z * (1.0f - u) + q1.z * u;
		result.w = this->w * (1.0f - u) + q1.w * u;
		return result;
	}

	quatf quatf::cubic_mix(quatf& q1, quatf& q2, quatf& q3, float u)
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
	}


}