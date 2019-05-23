#include "OEMaths_quat.h"
#include "OEMaths.h"
#include <cassert>

namespace OEMaths
{
	quatf::quatf(const float* data)
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

	quatf::quatf(const double* data)
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

	void quatf::linearMix(quatf& q1, quatf& q2, float u)
	{
		this->x = q1.x * (1.0f - u) + q2.x * u;
		this->y = q1.y * (1.0f - u) + q2.y * u;
		this->z = q1.z * (1.0f - u) + q2.z * u;
		this->w = q1.w * (1.0f - u) + q2.w * u;
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