#include "OEMaths_Mat2.h"
#include "OEMaths/OEMaths_Vec2.h"

namespace OEMaths
{
    float& mat2f::operator()(const uint8_t& col, const uint8_t& row)
	{
		// col major
		return data[col * 1 + row];
	}

	mat2f& mat2f::operator()(vec2f& vec, const uint8_t& col)
	{
		mat2f result;
		result.data[col * 2] = vec.getX();
		result.data[col * 2 + 1] = vec.getY();
		return result;
	}
}