#include "OEMaths_Vec3.h"

#include <assert.h>
#include <algorithm>
#include <cmath>

namespace OEMaths
{
    void vec3f::convert_vec3_F(const float* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;

		this->x = *ptr;
		++ptr;
		this->y = *ptr;
		++ptr;
		this->z = *ptr;
	}

	void vec3f::convert_vec3_D(const double* data)
	{
		assert(data != nullptr);
		double* ptr = (double*)data;

		this->x = (float)*ptr;
		++ptr;
		this->y = (float)*ptr;
		++ptr;
		this->z = (float)*ptr;
	}

    float vec3f::length()
	{
		return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
	}

	void vec3f::normalise()
	{
		float l = length();

		this->x / l;
		this->y / l;
		this->z / l;
	}

    vec3f vec3f::cross(vec3f& v1)
	{
		vec3f result;
		result.x = this->y * v1.getZ() - this->z * v1.getY();
		result.y = this->z * v1.getX() - this->x * v1.getZ();
		result.z = this->x * v1.getY() - this->y * v1.getX();

		return result;
	}

	float vec3f::dot(vec3f& v1)
	{
		return this->x * v1.getX() + this->y * v1.getY() + this->z * v1.getZ();
	}

	vec3f vec3f::mix(vec3f& v1, float u)
	{
		vec3f result;
        result.x = this->x * (1.0f - u) + v1.getX() * u;
		result.y = this->y * (1.0f - u) + v1.getY() * u;
		result.z = this->z * (1.0f - u) + v1.getZ() * u;
	}
}