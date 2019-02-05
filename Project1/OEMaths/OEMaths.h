#pragma once

#include <assert.h>
#include <cstdint>

#define M_PI 3.14159265358979323846264338327950288419716939937510582097

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

		vec3& operator-(const vec3& other)
		{
			x = x - other.x;
			y = y - other.y;
			z = z - other.z;
			return *this;
		}

		vec3& operator+(const vec3& other)
		{
			x = x + other.x;
			y = y + other.y;
			z = z + other.z;
			return *this;
		}

		vec3& operator*(const vec3& other)
		{
			x = x * other.x;
			y = y * other.y;
			z = z * other.z;
		}

		vec3& operator*(const vec4<T>& other)
		{
			x = x * other.x;
			y = y * other.y;
			z = z * other.z;
		}

		vec3& operator*(const float& other)
		{
			x = x * other;
			y = y * other;
			z = z * other;
			return *this;
		}

		vec3& operator*(const mat4<T>& other)
		{
			x = other[0] * x + other[1] * y + other[2] * z;
			y = other[4] * x + other[5] * y + other[6] * z;
			z = other[8] * x + other[9] * y + other[10] * z;
			return *this;
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
			return data[a * 2 + b];
		}

		mat3& operator()(const vec3<T>& vec, const uint8_t& row)
		{
			data[row * 2] = vec.x;
			data[row * 2 + 1] = vec.y;
			data[row * 2 + 2] = vec.z;
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
			data[0] = static_cast<T>(1);
			data[5] = static_cast<T>(1);
			data[10] = static_cast<T>(1);
			data[15] = static_cast<T>(1);
		}

		T& operator()(const uint8_t& a, const uint8_t& b)
		{
			return data[a * 3 + b];
		}

		mat4& operator()(const vec4<T>& vec, const uint8_t& row)
		{
			data[row * 3] = vec.x;
			data[row * 3 + 1] = vec.y;
			data[row * 3 + 2] = vec.z;
			data[row * 3 + 3] = vec.w;
		}

		mat4& operator*(const mat4<T>& other)
		{
			for (int y = 0; y < 3; ++y) {
				for (int x = 0; x < 3; ++x) {
					data[y * 3 + x] = other.data[y * 3 + x];
				}
			}
			return *this;
		}

		/*friend mat4<T> operator*(mat4<T> lhs, const mat4<T>& rhs)
		{
			for (int y = 0; y < 3; ++y) {
				for (int x = 0; x < 3; ++x) {
					lhs.data[y * 3 + x] = rhs.data[y * 3 + x];
				}
			}
			return lhs;
		}*/

	private:

		T data[16];
	};

	using mat4f = mat4<float>;
	using mat4d = mat4<double>;

}
	
