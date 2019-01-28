#pragma once

namespace OEMaths
{

	template<typename Typename>
	class mat2
	{
	public:

		mat2() :
			data[0](static_cast<Typename>(1)),
			data[3](static_cast<Typename>(1))
		{}

		Typename& operator()(const uint8_t& a, const uint8_t& b)
		{
			return data[a * 1 + b];
		}

		vec2<Typename>& operator()(const vec2<Typename>& vec, const uint8_t& row)
		{
			data[row * 1](vec.x);
			data[row * 1 + 1](vec.y);
		}

	private:

		Typename data[4];
	};

	using mat2f = mat2<float>;
	using mat2d = mat2<double>;
}
