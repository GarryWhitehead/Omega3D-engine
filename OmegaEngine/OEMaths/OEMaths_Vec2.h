#pragma once

#include <cstdint>

namespace OEMaths
{
// forward declerations
class vec2f;
class vec3f;
class vec4f;
class mat2f;
class mat3f;
class mat4f;

template<typename T>
class vec2
{
public:
	vec2() :
        x(0.0f),
        y(0.0f)
	{
	}

	vec2(T n)
	    : x(n)
	    , y(n)
	{
	}

	vec2(T in_x, T in_y)
	    : x(in_x)
	    , y(in_y)
	{
	}
    
	vec2(const float *data);
	vec2(const double *data);
    
	// operator overloads
	vec2 operator*(const vec2 &other) const;
	vec2 operator*(const float &other) const;
    vec2f operator/(const vec2f &other) const;
    vec2f operator/(const float &other) const;
    vec2f& operator+=(const vec2f& other);
    
    static constexpr size_t vecSize = 2;
    
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

} // namespace OEMaths
