#pragma once
#include <cmath>

namespace OEMaths
{
	class mat4f;

	class quatf
	{
	public:

		quatf() :
			x(0.0f),
			y(0.0f),
			z(0.0f),
			w(0.0f)
		{}

		quatf(float n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		quatf(float in_x, float in_y, float in_z, float in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}
		
		quatf(const float* data);
		quatf(const double* data);

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

        float getW() const
        {
            return w;
        }

		float length();
		void normalise();
		void linear_mix(quatf& q1, quatf& q2, float u);
		quatf cubic_mix(quatf& q1, quatf& q2, quatf& q3, float u);

	private:

		float x;
		float y;
		float z;
		float w;
	};

}