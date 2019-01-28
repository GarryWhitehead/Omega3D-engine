#pragma once

#include <assert.h>

namespace OEMaths
{

	template<typename Typename>
	class vec4
	{
	public:

		vec4() :
			x(static_cast<Typename>(0)),
			y(static_cast<Typename>(0)),
			z(static_cast<Typename>(0)),
			w(static_cast<Typename>(0))
		{}

		vec4(vec2<Typename> vec, Typename _z, Typename _w) :
			x(vec.x),
			y(vec.y),
			z(_z),
			w(_w)
		{}

		vec4(vec3<Typename> vec, Typename _w) :
			x(vec.x),
			y(vec.y),
			z(vec.z),
			w(_w)
		{}

		vec4(Typename n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		vec4(Typename in_x, Typename in_y, Typename in_z, Typename in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}

		Typename x;
		Typename y;
		Typename z;
		Typename w;
	};

	using vec4f = vec4<float>;
	using vec4d = vec4<double>;

	template <typename Typename>
	vec4<Typename> convert_vec4(Typename* data)
	{
		assert(data != nullptr);

		vec4<Typename> vec;
		vec.x = *data;
		data += sizeof(Typename);
		vec.y = *data;
		data += sizeof(Typename);
		vec.z = *data;
		data += sizeof(Typename);
		vec.w = *data;
		return vec;
	}

	template <typename Typename>
	inline vec4<Typename> length_vec4(vec4<Typename>& v4)
	{
		return std::sqrt(v4.x * v4.x + v4.y * v4.y + v4.z * v4.z + v4.w * v4.w);
	}

	template <typename Typename>
	inline vec4<Typename> normalise_vec4(vec4<Typename>& v4)
	{
		vec4<Typename> retVec;
		Typename length = length_vec4(v4);
		Typename invLength = static_cast<Typename>(1) / length;

		retVec.x = v4.x * invLength;
		retVec.y = v4.y * invLength;
		retVec.z = v4.z * invLength;
		retVec.w = v4.w * invLength;
		return retVec;
	}

	// interpolation =========================================================

	template <typename Typename>
	inline vec4<Typename> mix_vec4(vec4<Typename>& v1, vec4<Typename>& v2, float u)
	{
		vec4<Typename> retVec;
		retVec.x = v1.x * (1 - u) + v2.x * u;
		retVec.y = v1.y * (1 - u) + v2.y * u;
		retVec.z = v1.z * (1 - u) + v2.z * u;
		retVec.w = v1.w * (1 - u) + v2.w * u;
		return retVec;
	}


}