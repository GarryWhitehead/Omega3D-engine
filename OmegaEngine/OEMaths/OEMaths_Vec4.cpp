
#include "OEMaths_Vec4.h"
#include "OEMaths/OEMaths_Vec3.h"

#include <algorithm>
#include <assert.h>
#include <cmath>

namespace OEMaths
{
vec4f::vec4f(const float *data)
{
	assert(data != nullptr);
	float *ptr = (float *)data;

	this->x = *ptr;
	++ptr;
	this->y = *ptr;
	++ptr;
	this->z = *ptr;
	++ptr;
	this->w = *ptr;
}

vec4f::vec4f(const double *data)
{
	assert(data != nullptr);
	double *ptr = (double *)data;

	this->x = (float)*ptr;
	++ptr;
	this->y = (float)*ptr;
	++ptr;
	this->z = (float)*ptr;
	++ptr;
	this->w = (float)*ptr;
}

vec4f::vec4f(const uint16_t *data)
{
	assert(data != nullptr);
	uint16_t *ptr = (uint16_t *)data;

	this->x = (float)*ptr;
	++ptr;
	this->y = (float)*ptr;
	++ptr;
	this->z = (float)*ptr;
	++ptr;
	this->w = (float)*ptr;
}

vec4f vec4f::operator*(const vec4f &other) const
{
	vec4f result;
	result.x = x * other.x;
	result.y = y * other.y;
	result.z = z * other.z;
	result.w = w * other.w;
	return result;
}

float vec4f::length()
{
	return std::sqrt(this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w);
}

void vec4f::normalise()
{
	float l = length();

	this->x = this->x / l;
	this->y = this->y / l;
	this->z = this->z / l;
	this->w = this->w / l;
}

void vec4f::mix(vec4f &v1, vec4f &v2, float u)
{
	this->x = v1.x * (1.0f - u) + v2.x * u;
	this->y = v1.y * (1.0f - u) + v2.y * u;
	this->z = v1.z * (1.0f - u) + v2.z * u;
	this->w = v1.w * (1.0f - u) + v2.w * u;
}
} // namespace OEMaths