#pragma once

#include <cstdint>

namespace OEMaths
{
class vec3f;

class mat3f
{
public:
	mat3f()
	{
		data[0] = 1.0f;
		data[4] = 1.0f;
		data[8] = 1.0f;
	}

	float &operator()(const uint8_t &col, const uint8_t &row);
	mat3f &operator()(const vec3f &vec, const uint8_t &col);

private:
	float data[9];
};

} // namespace OEMaths