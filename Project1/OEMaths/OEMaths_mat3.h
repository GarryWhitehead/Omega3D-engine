#pragma once


#include <assert.h>

namespace OEMaths
{
	template<typename Typename>
	class mat3
	{
	public:

		mat3() :
			data[0](static_cast<Typename>(1)),
			data[4](static_cast<Typename>(1)),
			data[8](static_cast<Typename>(1))
		{}

		Typename& operator()(Typename& a, Typename& b)
		{
			return data[a * 2 + b];
		}

		mat3<Typename>& operator()(const vec3<Typename>& vec, const uint8_t& row)
		{
			data[row * 2] = vec.x;
			data[row * 2 + 1] = vec.y;
			data[row * 2 + 2] = vec.z;
		}

	private:

		Typename data[9];
	};


	using mat3f = mat3<float>;
	using mat3d = mat3<double>;
}