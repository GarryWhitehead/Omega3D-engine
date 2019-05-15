#include "OEMaths_Vec3.h"
#include "OEMaths/OEMaths_Vec4.h"
#include "OEMaths/OEMaths_Mat4.h"

#include <assert.h>
#include <algorithm>
#include <cmath>

namespace OEMaths
{
    vec3f vec3f::operator-(const vec3f& other) const
	{
		vec3f result;
		result.x = this->x - other.x;
		result.y = this->y - other.y;
		result.z = this->z - other.z;
		return result;
	}

	vec3f vec3f::operator+(const vec3f& other) const
	{
		vec3f result;
		result.x = this->x + other.x;
		result.y = this->y + other.y;
		result.z = this->z + other.z;
		return result;
	}

	vec3f vec3f::operator*(const vec3f& other) const
	{
		vec3f result;
		result.x = this->x * other.x;
		result.y = this->y * other.y;
		result.z = this->z * other.z;
		return result;
	}

	vec3f vec3f::operator*(vec4f& other) const
	{
		vec3f result;
		result.x = this->x * other.getX();
		result.y = this->y * other.getY();
		result.z = this->z * other.getZ();
		return result;
	}

	vec3f vec3f::operator*(const float& other) const
	{
		vec3f result;
		result.x = this->x * other;
		result.y = this->y * other;
		result.z = this->z * other;
		return result;
	}

	vec3f vec3f::operator*(const mat4f& other) const
	{
		vec3f result;
		result.x = other.data[0] * x + other.data[1] * y + other.data[2] * z;
		result.y = other.data[4] * x + other.data[5] * y + other.data[6] * z;
		result.z = other.data[8] * x + other.data[9] * y + other.data[10] * z;
		return result;
	}

	vec3f vec3f::operator/(const vec3f& other) const
	{
		vec3f result;
		result.x = x / other.x;
		result.y = y / other.y;
		result.z = z / other.z;
		return result;
	}

	vec3f& vec3f::operator-=(const vec3f& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	vec3f& vec3f::operator+=(const vec3f& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

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