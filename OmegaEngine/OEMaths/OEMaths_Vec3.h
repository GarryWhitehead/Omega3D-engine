#pragma once

#include "OEMaths/OEMaths_VecN"

namespace OEMaths
{
class vec4f;
class mat4f;

template<>
class vecN<float, 3>
{
public:
	vecN()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	vec3f(vec2f &vec, float f)
	    : x(vec.getX())
	    , y(vec.getY())
	    , z(f)
	{
	}

	vec3f(float n)
	    : x(n)
	    , y(n)
	    , z(n)
	{
	}

	vec3f(float in_x, float in_y, float in_z)
	    : x(in_x)
	    , y(in_y)
	    , z(in_z)
	{
	}

	vec3f(const float *data);
	vec3f(const double *data);

	vec3f operator-(const vec3f &other) const;
	vec3f operator+(const vec3f &other) const;
	constexpr vec3f operator*(const vec3f &other);
	vec3f operator*(vec4f &other) const;
	vec3f operator*(const float &other) const;
	vec3f operator*(const mat4f &other) const;
	vec3f operator/(const vec3f &other) const;
	vec3f &operator-=(const vec3f &other);
	vec3f &operator+=(const vec3f &other);

    friend vec3f operator*(const float& n, vec3f& v);
    
	float length();
	void normalise();
	vec3f cross(vec3f &v1);
	float dot(vec3f &v1);
	vec3f mix(vec3f &v1, float u);

public:
    
	 union
       {
           T v[vecSize];
           
           struct
           {
               T x;
               T y;
           };
               
           struct
           {
               T r;
               T g;
           };
       };
	
};

using vecN<float, 3> = vec3f;

} // namespace OEMaths
