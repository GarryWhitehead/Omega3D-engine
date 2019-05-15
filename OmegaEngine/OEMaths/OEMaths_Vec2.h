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

        float getX() const
        {
            return x;
        }

        float getY() const
        {
            return y;
        }

        void convert_F(const float* data);
        void convert_D(const double* data);

    private:

		float x;
		float y;
	};

}