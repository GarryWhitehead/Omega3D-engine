#pragma once

#include <cstdint>

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

		float& operator()(const uint8_t& col, const uint8_t& row);
		mat4f& operator()(const vec4f& vec, const uint8_t& col);
		mat4f& operator/=(const float& div);
		float& operator[](const uint32_t& index);
        
        friend mat4f operator*(const mat4f& m1, const mat4f& m2);
        friend vec4f operator*(const vec4f& vec, const mat4f& mat);
        friend vec4f operator*(const mat4f& mat, const vec4f& vec);
        friend vec3f vec3f::operator*(const mat4f& other) const;

		void setCol(const uint8_t col, vec4f& v);

        void convert_F(const float* data);
        void convert_D(const double* data);

        void translate(vec3f& trans);
        void scale(vec3f& scale);
        void rotate(float theta, vec3f& axis);
        bool inverse();

	private:

		float data[16];
	};

	vec4f operator*(const mat4f& mat, const vec4f& vec);
	vec4f operator*(const vec4f& vec, const mat4f& mat);
	mat4f operator*(const mat4f& m1, const mat4f& m2);
    
   
}