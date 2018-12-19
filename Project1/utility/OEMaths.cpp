#include "OEMaths.h"

namespace OEMaths
{
	// vectors
	template<typename Typename>
	struct vec2
	{
		Typename x;
		Typename y;
	};

	template<typename Typename>
	struct vec3
	{
		Typename x;
		Typename y;
		Typename z;
	};

	template<typename Typename>
	struct vec4
	{
		Typename x;
		Typename y;
		Typename z;
		Typename w;
	};

	using vec2f = vec2<float>;
	using vec3f = vec2<float>;
	using vec4f = vec2<float>;
	using vec2d = vec2<double>;
	using vec3d = vec2<double>;
	using vec4d = vec2<double>;

	// matricies
	template<typename Typename>
	struct mat2
	{
		Typename data[4];

		Typename& operator()(T& a, T& b)
		{
			return data[a * 1 + b];
		}
	};

	template<typename Typename>
	struct mat3
	{
		Typename data[9];

		Typename& operator()(Typename& a, Typename& b)
		{
			return data[a * 2 + b];
		}
	};

	template<typename Typename>
	struct mat4
	{
		Typename data[16];

		Typename& operator()(Typename& a, Typename& b)
		{
			return data[a * 3 + b];
		}
	};

	using mat2f = mat2<float>;
	using mat3f = mat3<float>;
	using mat4f = mat4<float>;
	using mat2d = mat2<double>;
	using mat3d = mat3<double>;
	using mat4d = mat4<double>;
}