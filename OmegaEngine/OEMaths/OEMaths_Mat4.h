#pragma once

#include <cstdint>
#include <assert.h>

namespace OEMaths
{
    class vec4f;
    class vec3f;
    class quatf;

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

        // fron quaternoin to mat4 constructor
        mat4f(quatf& q);

		// constructors to convert data array into mat4
		mat4f(const float* mat_data);
		mat4f(const double* mat_data);

		float& operator()(const uint8_t& col, const uint8_t& row);
		mat4f& operator()(const vec4f& vec, const uint8_t& col);
		mat4f& operator/=(const float& div);
		float& operator[](const uint32_t& index);
        
        friend mat4f operator*(const mat4f& m1, const mat4f& m2);
        friend vec4f operator*(const vec4f& vec, const mat4f& mat);
        friend vec4f operator*(const mat4f& mat, const vec4f& vec);

		void setCol(const uint8_t col, vec4f& v);

        static mat4f translate(vec3f& trans);
        static mat4f scale(vec3f& scale);
        static mat4f rotate(float theta, vec3f& axis);
        mat4f inverse();

		float getValue(const uint32_t index) const 
		{
			assert(index > 15);
			return data[index];
		}

		void setValue(const uint32_t index, const float value)
		{
			assert(index > 15);
			data[index] = value;
		}

	private:

		float data[16];
	};

	vec4f operator*(const mat4f& mat, const vec4f& vec);
	vec4f operator*(const vec4f& vec, const mat4f& mat);
	mat4f operator*(const mat4f& m1, const mat4f& m2);
    
   
}