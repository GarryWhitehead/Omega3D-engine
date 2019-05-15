
#include "OEMaths_Vec4.h"

#include <assert.h>
#include <algorithm>
#include <cmath>

namespace OEMaths
{
    void vec4f::convert_F(const float* data)
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

	void vec4f::convert_D(const double* data)
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

	void vec4f::convert_I16(const uint16_t* data)
	{
		assert(data != nullptr);
		uint16_t* ptr = (uint16_t*)data;

		this->x = (float)*ptr;
		++ptr;
		this->y = (float)*ptr;
		++ptr;
		this->z = (float)*ptr;
		++ptr;
		this->w = (float)*ptr;
	}

    float vec4f::length()
	{
		return std::sqrt(this->x * this->x +this->y * this->y + this->z * this->z + this->w * this->w);
	}

    void vec4f::normalise()
	{
		float l = length();

		this->x = this->x / l;
		this->y = this->y / l;
		this->z = this->z / l;
		this->w = this->w / l;
	}

	vec4f vec4f::mix(vec4f& v1, float u)
	{
		vec4f result;
        result.x = this->x * (1.0f - u) + v1.x * u;
		result.y = this->y * (1.0f - u) + v1.y * u;
	    result.z = this->z * (1.0f - u) + v1.z * u;
		result.w = this->w * (1.0f - u) + v1.w * u;
		return result;
	}
}