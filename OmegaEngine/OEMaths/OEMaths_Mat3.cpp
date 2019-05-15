#include "OEMaths_Mat3.h"
#include "OEMaths/OEMaths_Vec3.h"

namespace OEMaths
{
    float& mat3f::operator()(const uint8_t& col, const uint8_t& row)
	{
		return data[col * 3 + row];
	}

	mat3f& mat3f::operator()(const vec3f& vec, const uint8_t& col)
	{
		data[col * 3] = vec.getX();
		data[col * 3 + 1] = vec.getY();
		data[col * 3 + 2] = vec.getZ();
	}
}