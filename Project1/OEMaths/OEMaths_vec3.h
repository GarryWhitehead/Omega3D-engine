#pragma once

#include <assert.h>

namespace OEMaths
{
	
	template<typename Typename>
	class vec3
	{
	public:

		vec3() :
			x(static_cast<Typename>(0)),
			y(static_cast<Typename>(0)),
			z(static_cast<Typename>(0))
		{}

		vec3(vec2<Typename> vec, Typename f) :
			x(vec.x),
			y(vec.y),
			z(f)
		{}

		vec3(Typename n) :
			x(n),
			y(n),
			z(n)
		{}

		vec3(Typename in_x, Typename in_y, Typename in_z) :
			x(in_x),
			y(in_y),
			z(in_z)
		{}

		Typename x;
		Typename y;
		Typename z;
	};

	using vec3f = vec3<float>;
	using vec3d = vec3<double>;

	template <typename Typename>
	vec3<Typename> convert_vec3(Typename* data)
	{
		assert(data != nullptr);

		vec3<Typename> vec;
		vec.x = *data;
		data += sizeof(Typename);
		vec.y = *data;
		data += sizeof(Typename);
		vec.z = *data;
		return vec;
	}


	// coversion from one vec type to another
	template <typename Typename>
	vec4<Typename> vec3_to_vec4(vec3<Typename> v3, Typename w)
	{
		vec4<Typename> v4;
		v4.x = v3.x;
		v4.y = v3.y;
		v4.z = v3.z;
		v4.w = w;
		return v4;
	}

	// Vector math functions ==============================================================

	template <typename Typename>
	inline vec3<Typename> length_vec3(vec4<Typename>& v3)
	{
		return std::sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
	}

	template <typename Typename>
	vec3<Typename> normalise_vec3(vec3<Typename>& v3)
	{
		vec3<Typename> retVec;
		Typename length = length_vec3(v3);
		Typename invLength = static_cast<Typename>(1) / length;

		retVec.x = v3.x * invLength;
		retVec.y = v3.y * invLength;
		retVec.z = v3.z * invLength;
		return retVec;
	}


	// interpolation ===================================================================

	template <typename Typename>
	inline vec4<Typename> mix_vec3(vec3<Typename>& v1, vec3<Typename>& v2, float u)
	{
		vec3<Typename> retVec;
		retVec.x = v1.x * (1 - u) + v2.x * u;
		retVec.y = v1.y * (1 - u) + v2.y * u;
		retVec.z = v1.z * (1 - u) + v2.z * u;
		return retVec;
	}

}