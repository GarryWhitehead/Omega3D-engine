#pragma once

#include <assert.h>

namespace OEMaths
{
	template<typename Typename>
	class vec2
	{
	public:

		vec2() :
			x(static_cast<Typename>(0)),
			y(static_cast<Typename>(0))
		{}

		vec2(Typename n) :
			x(n),
			y(n)
		{}

		vec2(Typename in_x, Typename in_y) :
			x(in_x),
			y(in_y)
		{}

		Typename x;
		Typename y;
	};


	using vec2f = vec2<float>;
	using vec2d = vec2<double>;

	// vector conversion ================================================================

	template <typename Typename>
	vec2<Typename> convert_vec2(Typename* data)
	{
		assert(data != nullptr);

		vec2<Typename> vec;
		vec.x = *data;
		data += sizeof(Typename);
		vec.y = *data;;
		return vec;
	}

}