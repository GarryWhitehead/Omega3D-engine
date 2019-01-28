#pragma once

#include <assert.h>

namespace OEMaths
{
	// vectors
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

	using vec2f = vec2<float>;
	using vec3f = vec3<float>;
	using vec4f = vec4<float>;
	using vec2d = vec2<double>;
	using vec3d = vec3<double>;
	using vec4d = vec4<double>;

	// vector conversion
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


	// matricies
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

	using mat2f = mat2<float>;
	using mat3f = mat3<float>;
	using mat4f = mat4<float>;
	using mat2d = mat2<double>;
	using mat3d = mat3<double>;
	using mat4d = mat4<double>;

	// matrix conversion 
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

	// Vector math functions
	template <typename Typename>
	inline vec3<Typename> length_vec3(vec4<Typename>& v3)
	{
		return std::sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
	}

	template <typename Typename>
	inline vec4<Typename> length_vec4(vec4<Typename>& v4)
	{
		return std::sqrt(v4.x * v4.x + v4.y * v4.y + v4.z * v4.z + v4.w * v4.w);
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

	template <typename Typename>
	inline vec4<Typename> mix_vec3(vec3<Typename>& v1, vec3<Typename>& v2, float u)
	{
		vec3<Typename> retVec;
		retVec.x = v1.x * (1 - u) + v2.x * u;
		retVec.y = v1.y * (1 - u) + v2.y * u;
		retVec.z = v1.z * (1 - u) + v2.z * u;
		return retVec;
	}

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

