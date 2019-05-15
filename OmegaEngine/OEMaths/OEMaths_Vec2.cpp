
#include "OEMaths_Vec2.h"

#include <assert.h>

namespace OEMaths
{
	void vec2f::convert_F(const float* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;

		this->x = *ptr;
		++ptr;
		this->y = *ptr;
	}

	void vec2f::convert_D(const double* data)
	{
		assert(data != nullptr);
		double* ptr = (double*)data;

		this->x = (float)*ptr;
		++ptr;
		this->y = (float)*ptr;
	}
}