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

		float x;
		float y;
		float z;
		float w;
	};

	// quarternoin static functions
	quatf convert_quatf_F(const float* data);
	quatf convert_quatf_D(const double* data);

	mat4f quat_to_mat4(quatf& q);
	float length_quat(quatf& q);
	quatf normalise_quat(quatf& q);
	quatf linear_mix_quat(quatf& q1, quatf& q2, float u);
	quatf cubic_mix_quat(quatf& q1, quatf& q2, quatf& q3, quatf& q4, float u);


}