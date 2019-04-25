#include "OEMaths.h"

namespace OEMaths
{

	// =========================================== vec3 ====================================================================================

	vec3f vec3f::operator-(const vec3f& other) const
	{
		vec3f result;
		result.x = x - other.x;
		result.y = y - other.y;
		result.z = z - other.z;
		return result;
	}

	vec3f vec3f::operator+(const vec3f& other) const
	{
		vec3f result;
		result.x = x + other.x;
		result.y = y + other.y;
		result.z = z + other.z;
		return result;
	}

	vec3f vec3f::operator*(const vec3f& other) const
	{
		vec3f result;
		result.x = x * other.x;
		result.y = y * other.y;
		result.z = z * other.z;
		return result;
	}

	vec3f vec3f::operator*(const vec4f& other) const
	{
		vec3f result;
		result.x = x * other.x;
		result.y = y * other.y;
		result.z = z * other.z;
		return result;
	}

	vec3f vec3f::operator*(const float& other) const
	{
		vec3f result;
		result.x = x * other;
		result.y = y * other;
		result.z = z * other;
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

	// ================================================= vec4 ===============================================================
	

	vec4f vec4f::operator*(const vec4f& other) const
	{
		vec4f result;
		result.x = x * other.x;
		result.y = y * other.y;
		result.z = z * other.z;
		result.w = w * other.w;
		return result;
	}

	// matrices ==============================================================================================================================
	// mat2 =========================================

	float& mat2f::operator()(const uint8_t& col, const uint8_t& row)
	{
		// col major
		return data[col * 1 + row];
	}

	vec2f& mat2f::operator()(const vec2f& vec, const uint8_t& col)
	{
		data[col * 2] = vec.x;
		data[col * 2 + 1] = vec.y;
	}

	// ===================================================== mat3 =================================================================================

	float& mat3f::operator()(const uint8_t& col, const uint8_t& row)
	{
		return data[col * 3 + row];
	}

	mat3f& mat3f::operator()(const vec3f& vec, const uint8_t& col)
	{
		data[col * 3] = vec.x;
		data[col * 3 + 1] = vec.y;
		data[col * 3 + 2] = vec.z;
	}

	// ======================================= mat4 =========================================================

	float& mat4f::operator()(const uint8_t& col, const uint8_t& row)
	{
		// using col major
		assert(row < 4 && col < 4);
		return data[col * 4 + row];
	}

	mat4f& mat4f::operator()(const vec4f& vec, const uint8_t& col)
	{
		assert(col < 4);
		data[col * 4] = vec.x;
		data[col * 4 + 1] = vec.y;
		data[col * 4 + 2] = vec.z;
		data[col * 4 + 3] = vec.w;
		return *this;
	}

	mat4f& mat4f::operator/=(const float& div)
	{
		const float invDiv = 1 / div;
		data[0] /= div;
		data[1] /= div;
		data[2] /= div;
		data[3] /= div;

		data[4] /= div;
		data[5] /= div;
		data[6] /= div;
		data[7] /= div;

		data[8] /= div;
		data[9] /= div;
		data[10] /= div;
		data[11] /= div;

		data[12] /= div;
		data[13] /= div;
		data[14] /= div;
		data[15] /= div;
		return *this;
	}

	float& mat4f::operator[](const uint32_t& index)
	{
		return data[index];
	}

	void mat4f::setCol(const uint8_t col, vec4f& v)
	{
		assert(col < 4);
		uint8_t row = 0;
		data[col * 4 + row] = v.x;
		row++;
		data[col * 4 + row] = v.y;
		row++;
		data[col * 4 + row] = v.z;
		row++;
		data[col * 4 + row] = v.w;
	}

	vec4f operator*(const mat4f& mat, const vec4f& vec)
	{
		vec4f result;
		result.x = mat.data[0] * vec.x + mat.data[1] * vec.y + mat.data[2] * vec.z + mat.data[3] * vec.w;
		result.y = mat.data[4] * vec.x + mat.data[5] * vec.y + mat.data[6] * vec.z + mat.data[7] * vec.w;
		result.z = mat.data[8] * vec.x + mat.data[9] * vec.y + mat.data[10] * vec.z + mat.data[11] * vec.w;
		result.w = mat.data[12] * vec.x + mat.data[13] * vec.y + mat.data[14] * vec.z + mat.data[15] * vec.w;
		return result;
	}

	vec4f operator*(const vec4f& vec, const mat4f& mat)
	{
		vec4f result;
		result.x = vec.x * mat.data[0] + vec.y * mat.data[1] + vec.z * mat.data[2] + vec.w * mat.data[3];
		result.y = vec.x * mat.data[4] + vec.y * mat.data[5] + vec.z * mat.data[6] + vec.w * mat.data[7];
		result.z = vec.x * mat.data[8] + vec.y * mat.data[9] + vec.z * mat.data[10] + vec.w * mat.data[11];
		result.w = vec.x * mat.data[12] + vec.y * mat.data[13] + vec.z * mat.data[14] + vec.w * mat.data[15];
		return result;
	}

	mat4f operator*(const mat4f& m1, const mat4f& m2)
	{
		mat4f result;

		for (uint8_t row = 0; row < 4; ++row) {

			for (uint8_t col = 0; col < 4; ++col) {
				result.data[col * 4 + row] = 0;

				for (uint8_t k = 0; k < 4; ++k) {
					result.data[col * 4 + row] += m1.data[k * 4 + row] * m2.data[col * 4 + k];
				}
			}
		}

		return result;
	}

}

