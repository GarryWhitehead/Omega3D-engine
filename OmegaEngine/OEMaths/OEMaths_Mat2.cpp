#include "OEMaths_Mat2.h"
#include "OEMaths/OEMaths_Vec2.h"

namespace OEMaths
{
    float& mat2f::operator()(const uint8_t& col, const uint8_t& row)
	{
		// col major
		return data[col * 1 + row];
	}

	vec2f& mat2f::operator()(vec2f& vec, const uint8_t& col)
	{
		data[col * 2] = vec.getX();
		data[col * 2 + 1] = vec.getY();
	}
}