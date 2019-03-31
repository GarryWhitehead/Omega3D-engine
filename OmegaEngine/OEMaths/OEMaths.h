#pragma once

#include <assert.h>
#include <cstdint>

#define M_PI 3.14159265358979323846264338327950288419716939937510582097
#define EPSILON 0.00001

namespace OEMaths
{
	template<typename T> class mat4;
	template<typename T> class mat3;
	template<typename T> class mat2;
	template<typename T> class vec4;
	template<typename T> class vec3;
	template<typename T> class vec2;


	// vec2 ==============================================

	template<typename T>
	class vec2
	{
	public:

		vec2()
		{
			x = T(0);
			y = T(0);
		}

		vec2(T n) :
			x(n),
			y(n)
		{}

		vec2(T in_x, T in_y) :
			x(in_x),
			y(in_y)
		{}

		T x;
		T y;
	};


	using vec2f = vec2<float>;
	using vec2d = vec2<double>;

	// =========================================== vec3 ====================================================================================

	template<typename T>
	class vec3
	{
	public:

		vec3()
		{
			x = T(0);
			y = T(0);
			z = T(0);
		}

		vec3(vec2<T> vec, T f) :
			x(vec.x),
			y(vec.y),
			z(f)
		{}

		vec3(T n) :
			x(n),
			y(n),
			z(n)
		{}

		vec3(T in_x, T in_y, T in_z) :
			x(in_x),
			y(in_y),
			z(in_z)
		{}

		vec3 operator-(const vec3& other) const
		{
			vec3 result;
			result.x = x - other.x;
			result.y = y - other.y;
			result.z = z - other.z;
			return result;
		}

		vec3 operator+(const vec3& other) const
		{
			vec3 result;
			result.x = x + other.x;
			result.y = y + other.y;
			result.z = z + other.z;
			return result;
		}

		vec3 operator*(const vec3& other) const
		{
			vec3 result;
			result.x = x * other.x;
			result.y = y * other.y;
			result.z = z * other.z;
			return result;
		}

		vec3 operator*(const vec4<T>& other) const
		{
			vec3 result;
			result.x = x * other.x;
			result.y = y * other.y;
			result.z = z * other.z;
			return result;
		}

		vec3 operator*(const float& other) const
		{
			vec3 result;
			result.x = x * other;
			result.y = y * other;
			result.z = z * other;
			return result;
		}

		vec3 operator*(const mat4<T>& other) const
		{
			vec3 result;
			result.x = other[0] * x + other[1] * y + other[2] * z;
			result.y = other[4] * x + other[5] * y + other[6] * z;
			result.z = other[8] * x + other[9] * y + other[10] * z;
			return result;
		}

		vec3 operator/(const vec3& other) const
		{
			vec3 result;
			result.x = x / other.x;
			result.y = y / other.y;
			result.z = z / other.z;
			return result;
		}

		vec3& operator-=(const vec3& other)
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
		T x;
		T y;
		T z;

	};

	using vec3f = vec3<float>;
	using vec3d = vec3<double>;

	// ================================================= vec4 ===============================================================
	template <typename T>
	class vec4
	{
	public:

		vec4()
		{
			x = T(0);
			y = T(0);
			z = T(0);
			w = T(0);
		}

		vec4(vec2<T> vec, T _z, T _w) :
			x(vec.x),
			y(vec.y),
			z(_z),
			w(_w)
		{}

		vec4(vec3<T> vec, T _w) :
			x(vec.x),
			y(vec.y),
			z(vec.z),
			w(_w)
		{}

		vec4(T n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		vec4(T in_x, T in_y, T in_z, T in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}

		vec4 operator*(const vec4<T>& other) const
		{
			vec4 result;
			result.x = x * other.x;
			result.y = y * other.y;
			result.z = z * other.z;
			result.w = w * other.w;
			return result;
		}

		// data
		T x;
		T y;
		T z;
		T w;
	};

	using vec4f = vec4<float>;
	using vec4d = vec4<double>;



	// matrices ==============================================================================================================================
	// mat2 =========================================

	template<typename T>
	class mat2
	{
	public:

		mat2()
		{
			data[0] = T(1);
			data[3] = T(1);
		}

		T& operator()(const uint8_t& col, const uint8_t& row)
		{
			// col major
			return data[col * 1 + row];
		}

		vec2<T>& operator()(const vec2<T>& vec, const uint8_t& col)
		{
			data[col * 2] = vec.x;
			data[col * 2 + 1] = vec.y;
		}

	private:

		T data[4];
	};

	using mat2f = mat2<float>;
	using mat2d = mat2<double>;

	// ===================================================== mat3 =================================================================================

	template<typename T>
	class mat3
	{
	public:

		mat3()
		{
			data[0] = T(1);
			data[4] = T(1);
			data[8] = T(1);
		}

		T& operator()(const uint8_t& col, const uint8_t& row)
		{
			return data[col * 3 + row];
		}

		mat3& operator()(const vec3<T>& vec, const uint8_t& col)
		{
			data[col * 3] = vec.x;
			data[col * 3 + 1] = vec.y;
			data[col * 3 + 2] = vec.z;
		}

	private:

		T data[9];
	};


	using mat3f = mat3<float>;
	using mat3d = mat3<double>;

	// ======================================= mat4 =========================================================

	template<typename T>
	class mat4
	{
	public:

		mat4()
		{
			data[0] = T(1);		data[1] = T(0);		data[2] = T(0);		data[3] = T(0);
			data[4] = T(0);		data[5] = T(1);		data[6] = T(0);		data[7] = T(0);
			data[8] = T(0);		data[9] = T(0);		data[10] = T(1);	data[11] = T(0);
			data[12] = T(0);	data[13] = T(0);	data[14] = T(0);	data[15] = T(1);
		}

		T& operator()(const uint8_t& col, const uint8_t& row)
		{
			// using col major
			assert(row < 4 && col < 4);
			return data[col * 4 + row];
		}

		inline mat4& operator()(const vec4<T>& vec, const uint8_t& col)
		{
			assert(col < 4);
			data[col * 4] = vec.x;
			data[col * 4 + 1] = vec.y;
			data[col * 4 + 2] = vec.z;
			data[col * 4 + 3] = vec.w;
			return *this;
		}

		inline mat4& operator/=(const T& div)
		{
			const T invDiv = 1 / div;
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

		T& operator[](const uint32_t& index)
		{
			return data[index];
		}

		template <typename T>
		void setCol(const uint8_t col, vec4<T>& v)
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

		T data[16];
	};

	using mat4f = mat4<float>;
	using mat4d = mat4<double>;

	template <typename T>
	inline vec4<T> operator*(const mat4<T> mat, const vec4<T>& vec)
	{
		vec4<T> result;
		result.x = mat.data[0] * vec.x + mat.data[1] * vec.y + mat.data[2] * vec.z + mat.data[3] * vec.w;
		result.y = mat.data[4] * vec.x + mat.data[5] * vec.y + mat.data[6] * vec.z + mat.data[7] * vec.w;
		result.z = mat.data[8] * vec.x + mat.data[9] * vec.y + mat.data[10] * vec.z + mat.data[11] * vec.w;
		result.w = mat.data[12] * vec.x + mat.data[13] * vec.y + mat.data[14] * vec.z + mat.data[15] * vec.w;
		return result;
	}

	template <typename T>
	inline vec4<T> operator*(const vec4<T>& vec, const mat4<T> mat)
	{
		vec4<T> result;
		result.x = vec.x * mat.data[0] + vec.y * mat.data[1] + vec.z * mat.data[2] + vec.w * mat.data[3];
		result.y = vec.x * mat.data[4] + vec.y * mat.data[5] + vec.z * mat.data[6] + vec.w * mat.data[7];
		result.z = vec.x * mat.data[8] + vec.y * mat.data[9] + vec.z * mat.data[10] + vec.w * mat.data[11];
		result.w = vec.x * mat.data[12] + vec.y * mat.data[13] + vec.z * mat.data[14] + vec.w * mat.data[15];
		return result;
	}

	template <typename T>
	inline mat4<T> operator*(const mat4<T>& m1, const mat4<T>& m2)
	{
		mat4<T> result;

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
	
