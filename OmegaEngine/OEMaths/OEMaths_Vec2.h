#pragma once

namespace OEMaths
{
// forward declerations
class vec2f;
class vec3f;
class vec4f;
class mat2f;
class mat3f;
class mat4f;

class vec2f
{
public:
	vec2f()
	{
		x = 0.0f;
		y = 0.0f;
	}

	vec2f(float n)
	    : x(n)
	    , y(n)
	{
	}

	vec2f(float in_x, float in_y)
	    : x(in_x)
	    , y(in_y)
	{
	}

	vec2f(const float *data);
	vec2f(const double *data);

	// operator overloads
	vec2f operator*(const vec2f &other) const;
	vec2f operator*(const float &other) const;
    vec2f operator/(const vec2f &other) const;
    vec2f operator/(const float &other) const;
    vec2f& operator+=(const vec2f& other);
    
	float getX() const
	{
		return x;
	}

	float getY() const
	{
		return y;
	}

private:
	float x;
	float y;
};

} // namespace OEMaths
