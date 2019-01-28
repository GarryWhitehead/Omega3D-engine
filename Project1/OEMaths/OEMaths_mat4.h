#pragma once

#include <assert.h>

namespace OEMaths
{
	template<typename Typename>
	class mat4
	{
	public:

		mat4() :
			data[0](static_cast<Typename>(1)),
			data[5](static_cast<Typename>(1)),
			data[10](static_cast<Typename>(1)),
			data[15](static_cast<Typename>(1))
		{}

		Typename& operator()(Typename& a, Typename& b)
		{
			return data[a * 3 + b];
		}

		mat4<Typename>& operator()(const vec4<Typename>& vec, const uint8_t& row)
		{
			data[row * 3] = vec.x;
			data[row * 3 + 1] = vec.y;
			data[row * 3 + 2] = vec.z;
			data[row * 3 + 3] = vec.w;
		}

		mat4<Typename>& operator*(const mat4<Typename>& other)
		{
			for (int y = 0; y < 3; ++y) {
				for (int x = 0; x < 3; ++x) {
					data[y * 3 + x] = other.data[y * 3 + x];
				}
			}
			return *this;
		}

		friend mat4<Typename> operator*(mat4<Typename> lhs, const mat4<Typename>& rhs)
		{
			for (int y = 0; y < 3; ++y) {
				for (int x = 0; x < 3; ++x) {
					lhs.data[y * 3 + x] = rhs.data[y * 3 + x];
				}
			}
			return lhs;
		}

	private:

		Typename data[16];
	};

	using mat4f = mat4<float>;
	using mat4d = mat4<double>;

	// matrix conversion  ===============================================================================

	template <typename Typename>
	mat4<Typename> convert_mat4(Typename* data)
	{
		assert(data != nullptr);

		mat4<Typename> mat;

		for (uint8_t row = 0; row < 4; ++row) {

			vec4<Typename> vec;
			vec.x = *data;
			data += sizeof(Typename);
			vec.y = *data;
			data += sizeof(Typename);
			vec.z = *data;
			data += sizeof(Typename);
			vec.w = *data;

			mat(vec, row);
		}
		return mat;
	}

	// matrix TRS ========================================================================================
	template <typename Typename>
	inline mat4<Typename> translate(mat4<Typename>& mat, vec3<Typename>& trans)
	{
		mat4<Typename> retMat = mat;

		retMat(0, 3) = trans.x;
		retMat(1, 3) = trans.y;
		retMat(2, 3) = trans.x;
		retMat(3, 3) = 1.0f;
		return retMat;
	}

	template <typename Typename>
	inline mat4<Typename> scale(mat4<Typename>& mat, vec3<Typename>& scale)
	{
		mat4<Typename> retMat = mat;

		retMat(0, 0) = scale.x;
		retMat(1, 1) = scale.y;
		retMat(2, 2) = scale.x;
		retMat(3, 3) = 1.0f;
		return retMat;
	}

}