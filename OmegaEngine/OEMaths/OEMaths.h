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
	// forward declerations
	class vec2f;
	class vec3f;
	class vec4f;
	class mat2f;
	class mat3f;
	class mat4f;

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

		vec3f operator-(const vec3f& other) const;
		vec3f operator+(const vec3f& other) const;
		vec3f operator*(const vec3f& other) const;
		vec3f operator*(const vec4f& other) const;
		vec3f operator*(const float& other) const;
		vec3f operator*(const mat4f& other) const;
		vec3f operator/(const vec3f& other) const;
		vec3f& operator-=(const vec3f& other);
		vec3f& operator+=(const vec3f& other);

	public:

		// data
		float x;
		float y;
		float z;

	};

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

		vec4f operator*(const vec4f& other) const;

		// data
		float x;
		float y;
		float z;
		float w;
	};

	// matrices ==============================================================================================================================

	class mat2f
	{
	public:

		mat2f()
		{
			data[0] = 1.0f;
			data[3] = 1.0f;
		}

		float& operator()(const uint8_t& col, const uint8_t& row);
		vec2f& operator()(const vec2f& vec, const uint8_t& col);

	private:

		float data[4];
	};

	class mat3f
	{
	public:

		mat3f()
		{
			data[0] = 1.0f;
			data[4] = 1.0f;
			data[8] = 1.0f;
		}

		float& operator()(const uint8_t& col, const uint8_t& row);
		mat3f& operator()(const vec3f& vec, const uint8_t& col);

	private:

		float data[9];
	};

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

		float& operator()(const uint8_t& col, const uint8_t& row);
		mat4f& operator()(const vec4f& vec, const uint8_t& col);
		mat4f& operator/=(const float& div);
		float& operator[](const uint32_t& index);

		void setCol(const uint8_t col, vec4f& v);

		
		float data[16];
	};

	vec4f operator*(const mat4f& mat, const vec4f& vec);
	vec4f operator*(const vec4f& vec, const mat4f& mat);
	mat4f operator*(const mat4f& m1, const mat4f& m2);

}
	
