#pragma once

#include "OEMaths/OEMaths_Vec2.h"
#include "OEMaths/OEMaths_Vec3.h"

#include <cstdint>

namespace OEMaths
{
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
			x(vec.getX()),
			y(vec.getY()),
			z(_z),
			w(_w)
		{}

		vec4f(vec3f vec, float _w) :
			x(vec.getX()),
			y(vec.getY()),
			z(vec.getZ()),
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

        float getX()
        {
            return x;
        }

        float getY()
        {
            return y;
        }

        float getZ()
        {
            return z;
        }

        float getW()
        {
            return w;
        }

		vec4f operator*(const vec4f& other) const;

        void convert_F(const float* data);
        void convert_D(const double* data);
        void convert_I16(const uint16_t* data);

        float length();
        void normalise();
        vec4f mix(vec4f& v1, float u);

    private:

		// data
		float x;
		float y;
		float z;
		float w;
	};

}