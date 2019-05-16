#pragma once

#include "OEMaths/OEMaths_Vec2.h"

namespace OEMaths
{
    class vec4f;
    class mat4f;
    

    class vec3f
	{
	public:

		vec3f()
		{
			x = 0.0f;
			y = 0.0f;
			z = 0.0f;
		}

		vec3f(vec2f& vec, float f) :
			x(vec.getX()),
			y(vec.getY()),
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

		vec3f(const float* data);
		vec3f(const double* data);

		vec3f operator-(const vec3f& other) const;
		vec3f operator+(const vec3f& other) const;
		vec3f operator*(const vec3f& other) const;
		vec3f operator*(vec4f& other) const;
		vec3f operator*(const float& other) const;
		vec3f operator*(const mat4f& other) const;
		vec3f operator/(const vec3f& other) const;
		vec3f& operator-=(const vec3f& other);
		vec3f& operator+=(const vec3f& other);

        float getX() const
        {
            return x;
        }

        float getY() const
        {
            return y;
        }

        float getZ() const
        {
            return z;
        }

		void setX(const float x)
		{
			this->x = x;
		}

		void setY(const float y)
		{
			this->y = y;
		}
		
		void setZ(const float z)
		{
			this->z = z;
		}
		

        float length();
        void normalise();
        vec3f cross(vec3f& v1);
        float dot(vec3f& v1);
        vec3f mix(vec3f& v1, float u);

	private:

		// data
		float x;
		float y;
		float z;

	};
}