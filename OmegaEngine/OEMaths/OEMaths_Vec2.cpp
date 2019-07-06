
#include "OEMaths_Vec2.h"

#include <assert.h>

namespace OEMaths
{
vec2f::vec2f(const float *data)
{
	assert(data != nullptr);
	float *ptr = (float *)data;

	this->x = *ptr;
	++ptr;
	this->y = *ptr;
}

vec2f::vec2f(const double *data)
{
	assert(data != nullptr);
	double *ptr = (double *)data;

	this->x = (float)*ptr;
	++ptr;
	this->y = (float)*ptr;
}

vec2f vec2f::operator*(const vec2f &other) const
{
	vec2f result;
	result.x = this->x * other.x;
	result.y = this->y * other.y;
	return result;
}

vec2f vec2f::operator*(const float &other) const
{
	vec2f result;
	result.x = this->x * other;
	result.y = this->y * other;
	return result;
}
} // namespace OEMaths