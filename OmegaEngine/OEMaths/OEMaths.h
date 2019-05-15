/*
	A non-templeted maths library for speedier compile times.
	(Who needs double vectors anyway?!)
*/

#pragma once

#include <assert.h>
#include <cstdint>

#define M_PI 3.14159265358979323846264338327950288419716939937510582097
#define EPSILON 0.00001


	// =========================================== vec3 ====================================================================================

	

	
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
	
