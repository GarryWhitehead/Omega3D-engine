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
			x = static_cast<T>(0);
			y = static_cast<T>(0);
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
			x = static_cast<T>(0);
			y = static_cast<T>(0);
			z = static_cast<T>(0);
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
			x = static_cast<T>(0);
			y = static_cast<T>(0);
			z = static_cast<T>(0);
			w = static_cast<T>(0);
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
			data[0] = 1;
			data[3] = 1;
		}

		T& operator()(const uint8_t& a, const uint8_t& b)
		{
			return data[a * 1 + b];
		}

		vec2<T>& operator()(const vec2<T>& vec, const uint8_t& row)
		{
			data[row * 1] = vec.x;
			data[row * 1 + 1] = vec.y;
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
			data[0] = 1;
			data[4] = 1;
			data[8] = 1;
		}

		T& operator()(const uint8_t& a, const uint8_t& b)
		{
			return data[a * 3 + b];
		}

		mat3& operator()(const vec3<T>& vec, const uint8_t& row)
		{
			data[row * 3] = vec.x;
			data[row * 3 + 1] = vec.y;
			data[row * 3 + 2] = vec.z;
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
			data[0] = 1; data[1] = 0; data[2] = 0; data[3] = 0;
			data[4] = 0; data[5] = 1; data[6] = 0; data[7] = 0;
			data[8] = 0; data[9] = 0; data[10] = 1; data[11] = 0;
			data[12] = 0; data[13] = 0; data[14] = 0; data[15] = 1;
		}

		T& operator()(const uint8_t& row, const uint8_t& col)
		{
			assert(row < 4 && col < 4);
			return data[row * 4 + col];
		}

		inline mat4& operator()(const vec4<T>& vec, const uint8_t& row)
		{
			data[row * 4] = vec.x;
			data[row * 4 + 1] = vec.y;
			data[row * 4 + 2] = vec.z;
			data[row * 4 + 3] = vec.w;
			return *this;
		}

		inline mat4& operator/=(const T& div)
		{
			const T invDiv = 1 / div;
			data[0] *= invDiv;
			data[1] *= invDiv;
			data[2] *= invDiv;
			data[3] *= invDiv;

			data[4] *= invDiv;
			data[5] *= invDiv;
			data[6] *= invDiv;
			data[7] *= invDiv;

			data[8] *= invDiv;
			data[9] *= invDiv;
			data[10] *= invDiv;
			data[11] *= invDiv;

			data[12] *= invDiv;
			data[13] *= invDiv;
			data[14] *= invDiv;
			data[15] *= invDiv;
			return *this;
		}

		T& operator[](const uint32_t& index)
		{
			return data[index];
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

		result.data[0] = m1.data[0] * m2.data[0] + m1.data[1] * m2.data[4] + m1.data[2] * m2.data[8] +
			m1.data[3] * m2.data[12];

		result.data[1] = m1.data[0] * m2.data[1] + m1.data[1] * m2.data[5] + m1.data[2] * m2.data[9] +
			m1.data[3] * m2.data[13];

		result.data[2] = m1.data[0] * m2.data[2] + m1.data[1] * m2.data[6] + m1.data[2] * m2.data[10] +
			m1.data[3] * m2.data[14];

		result.data[3] = m1.data[0] * m2.data[3] + m1.data[1] * m2.data[7] + m1.data[2] * m2.data[11] +
			m1.data[3] * m2.data[15];


		result.data[4] = m1.data[4] * m2.data[0] + m1.data[5] * m2.data[4] + m1.data[6] * m2.data[8] +
			m1.data[7] * m2.data[12];

		result.data[5] = m1.data[4] * m2.data[1] + m1.data[5] * m2.data[5] + m1.data[6] * m2.data[9] +
			m1.data[7] * m2.data[13];

		result.data[6] = m1.data[4] * m2.data[2] + m1.data[5] * m2.data[6] + m1.data[6] * m2.data[10] +
			m1.data[7] * m2.data[14];

		result.data[7] = m1.data[4] * m2.data[3] + m1.data[5] * m2.data[7] + m1.data[6] * m2.data[11] +
			m1.data[7] * m2.data[15];


		result.data[8] = m1.data[8] * m2.data[0] + m1.data[9] * m2.data[4] + m1.data[10] * m2.data[8] +
			m1.data[11] * m2.data[12];

		result.data[9] = m1.data[8] * m2.data[1] + m1.data[9] * m2.data[5] + m1.data[10] * m2.data[9] +
			m1.data[11] * m2.data[13];

		result.data[10] = m1.data[8] * m2.data[2] + m1.data[9] * m2.data[6] + m1.data[10] * m2.data[10] +
			m1.data[11] * m2.data[14];

		result.data[11] = m1.data[8] * m2.data[3] + m1.data[9] * m2.data[7] + m1.data[10] * m2.data[11] +
			m1.data[11] * m2.data[15];


		result.data[12] = m1.data[12] * m2.data[0] + m1.data[13] * m2.data[4] + m1.data[14] * m2.data[8] +
			m1.data[15] * m2.data[12];

		result.data[13] = m1.data[12] * m2.data[1] + m1.data[13] * m2.data[5] + m1.data[14] * m2.data[9] +
			m1.data[15] * m2.data[13];

		result.data[14] = m1.data[12] * m2.data[2] + m1.data[13] * m2.data[6] + m1.data[14] * m2.data[10] +
			m1.data[15] * m2.data[14];

		result.data[15] = m1.data[12] * m2.data[3] + m1.data[13] * m2.data[7] + m1.data[14] * m2.data[11] +
			m1.data[15] * m2.data[15];


		return result;
	}

}
	
