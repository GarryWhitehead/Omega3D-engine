/*
	A non-templeted maths library for speedier compile times.
	(Who needs double vectors anyway?!)
*/

#pragma once

#include <assert.h>
#include <cstdint>

#define M_PI 3.14159265358979323846264338327950288419716939937510582097
#define EPSILON 0.00001

namespace OEMaths
{
	// vec2 ==============================================

	class vec2f
	{
	public:

		vec2f()
		{
			x = 0.0f;
			y = 0.0f;
		}

		vec2f(float n) :
			x(n),
			y(n)
		{}

		vec2f(float in_x, float in_y) :
			x(in_x),
			y(in_y)
		{}

		float x;
		float y;
	};


	// =========================================== vec3 ====================================================================================

	class vec3f
	{
	public:

		vec3f()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		vec3f(vec2f vec, float f) :
			x(vec.x),
			y(vec.y),
			z(f)
		{}

		vec3f(float n) :
			x(n),
			y(n),
			z(n)
		{}

		vec3f(float in_x, float in_y, float in_z) :
			x(in_x),
			y(in_y),
			z(in_z)
		{}

		vec3f operator-(const vec3f& other) const
		{
			vec3f result;
			result.x = x - other.x;
			result.y = y - other.y;
			result.z = z - other.z;
			return result;
		}

		vec3f operator+(const vec3f& other) const
		{
			vec3f result;
			result.x = x + other.x;
			result.y = y + other.y;
			result.z = z + other.z;
			return result;
		}

		vec3f operator*(const vec3f& other) const
		{
			vec3f result;
			result.x = x * other.x;
			result.y = y * other.y;
			result.z = z * other.z;
			return result;
		}

		vec3f operator*(const vec4f& other) const
		{
			vec3f result;
			result.x = x * other.x;
			result.y = y * other.y;
			result.z = z * other.z;
			return result;
		}

		vec3f operator*(const float& other) const
		{
			vec3f result;
			result.x = x * other;
			result.y = y * other;
			result.z = z * other;
			return result;
		}

		vec3f operator*(const mat4f& other) const
		{
			vec3f result;
			result.x = other[0] * x + other[1] * y + other[2] * z;
			result.y = other[4] * x + other[5] * y + other[6] * z;
			result.z = other[8] * x + other[9] * y + other[10] * z;
			return result;
		}

		vec3f operator/(const vec3f& other) const
		{
			vec3f result;
			result.x = x / other.x;
			result.y = y / other.y;
			result.z = z / other.z;
			return result;
		}

		vec3f& operator-=(const vec3f& other)
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		vec3& operator+=(const vec3& other)
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}



	public:

		// data
		float x;
		float y;
		float z;

	};

	// ================================================= vec4 ===============================================================
	class vec4f
	{
	public:

		vec4f()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
			w = 0.0f;
		}

		vec4f(vec2f& vec, float _z, float _w) :
			x(vec.x),
			y(vec.y),
			z(_z),
			w(_w)
		{}

		vec4f(vec3f vec, float _w) :
			x(vec.x),
			y(vec.y),
			z(vec.z),
			w(_w)
		{}

		vec4f(float n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		vec4f(float in_x, float in_y, float in_z, float in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}

		vec4f operator*(const vec4f& other) const
		{
			vec4f result;
			result.x = x * other.x;
			result.y = y * other.y;
			result.z = z * other.z;
			result.w = w * other.w;
			return result;
		}

		// data
		float x;
		float y;
		float z;
		float w;
	};

	// matrices ==============================================================================================================================
	// mat2 =========================================

	class mat2f
	{
	public:

		mat2f()
		{
			data[0] = 1.0f;
			data[3] = 1.0f;
		}

		float& operator()(const uint8_t& col, const uint8_t& row)
		{
			// col major
			return data[col * 1 + row];
		}

		vec2f& operator()(const vec2f& vec, const uint8_t& col)
		{
			data[col * 2] = vec.x;
			data[col * 2 + 1] = vec.y;
		}

	private:

		float data[4];
	};


	// ===================================================== mat3 =================================================================================

	class mat3f
	{
	public:

		mat3()
		{
			data[0] = 1.0f;
			data[4] = 1.0f;
			data[8] = 1.0f;
		}

		float& operator()(const uint8_t& col, const uint8_t& row)
		{
			return data[col * 3 + row];
		}

		mat3f& operator()(const vec3f& vec, const uint8_t& col)
		{
			data[col * 3] = vec.x;
			data[col * 3 + 1] = vec.y;
			data[col * 3 + 2] = vec.z;
		}

	private:

		float data[9];
	};

	// ======================================= mat4 =========================================================

	class mat4f
	{
	public:

		mat4f()
		{
			data[0] = 1.0f;		data[1] = 0.0f;		data[2] = 0.0f;		data[3] = 0.0f;
			data[4] = 0.0f;		data[5] = 1.0f;		data[6] = 0.0f;		data[7] = 0.0f;
			data[8] = 0.0f;		data[9] = 0.0f;		data[10] = 1.0f;	data[11] = 0.0f;
			data[12] = 0.0f;	data[13] = 0.0f;	data[14] = 0.0f;	data[15] = 1.0f;
		}

		float& operator()(const uint8_t& col, const uint8_t& row)
		{
			// using col major
			assert(row < 4 && col < 4);
			return data[col * 4 + row];
		}

		inline mat4f& operator()(const vec4f& vec, const uint8_t& col)
		{
			assert(col < 4);
			data[col * 4] = vec.x;
			data[col * 4 + 1] = vec.y;
			data[col * 4 + 2] = vec.z;
			data[col * 4 + 3] = vec.w;
			return *this;
		}

		inline mat4f& operator/=(const float& div)
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

		float& operator[](const uint32_t& index)
		{
			return data[index];
		}

		void setCol(const uint8_t col, vec4f& v)
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

		float data[16];
	};

	inline vec4f operator*(const mat4f& mat, const vec4f& vec)
	{
		vec4f result;
		result.x = mat.data[0] * vec.x + mat.data[1] * vec.y + mat.data[2] * vec.z + mat.data[3] * vec.w;
		result.y = mat.data[4] * vec.x + mat.data[5] * vec.y + mat.data[6] * vec.z + mat.data[7] * vec.w;
		result.z = mat.data[8] * vec.x + mat.data[9] * vec.y + mat.data[10] * vec.z + mat.data[11] * vec.w;
		result.w = mat.data[12] * vec.x + mat.data[13] * vec.y + mat.data[14] * vec.z + mat.data[15] * vec.w;
		return result;
	}

	inline vec4f operator*(const vec4f& vec, const mat4f& mat)
	{
		vec4f result;
		result.x = vec.x * mat.data[0] + vec.y * mat.data[1] + vec.z * mat.data[2] + vec.w * mat.data[3];
		result.y = vec.x * mat.data[4] + vec.y * mat.data[5] + vec.z * mat.data[6] + vec.w * mat.data[7];
		result.z = vec.x * mat.data[8] + vec.y * mat.data[9] + vec.z * mat.data[10] + vec.w * mat.data[11];
		result.w = vec.x * mat.data[12] + vec.y * mat.data[13] + vec.z * mat.data[14] + vec.w * mat.data[15];
		return result;
	}

	inline mat4f operator*(const mat4f& m1, const mat4f& m2)
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
	
