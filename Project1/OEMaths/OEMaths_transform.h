#pragma once

#include <assert.h>
#include <cstdint>

#include "OEMaths.h"

namespace OEMaths
{
	// vec2 conversion ================================================================

	template <typename T>
	inline vec2<T> convert_vec2(T* data)
	{
		assert(data != nullptr);

		vec2<T> vec;
		vec.x = *data;
		data += sizeof(T);
		vec.y = *data;
		return vec;
	}
	
	// vec3 transform functions ========================================================================================================
	
	template <typename T>
	inline vec3<T> convert_vec3(T* data)
	{
		assert(data != nullptr);

		vec3<T> vec;
		vec.x = *data;
		data += sizeof(T);
		vec.y = *data;
		data += sizeof(T);
		vec.z = *data;
		return vec;
	}


	// coversion from one vec type to another
	template <typename T>
	inline vec4<T> vec3_to_vec4(vec3<T> v3, T w)
	{
		vec4<T> v4;
		v4.x = v3.x;
		v4.y = v3.y;
		v4.z = v3.z;
		v4.w = w;
		return v4;
	}

	// Vector math functions ==============================================================

	template <typename T>
	inline T length_vec3(vec3<T>& v3)
	{
		return std::sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
	}

	template <typename T>
	inline vec3<T> normalise_vec3(vec3<T>& v3)
	{
		vec3<T> retVec;
		T length = length_vec3(v3);
		T invLength = static_cast<T>(1) / length;

		retVec.x = v3.x * invLength;
		retVec.y = v3.y * invLength;
		retVec.z = v3.z * invLength;
		return retVec;
	}

	template <typename T>
	inline vec3<T> cross_vec3(vec3<T>& v1, vec3<T>& v2)
	{
		vec3<T> retVec;
		retVec.x = v1.y * v2.z - v1.z * v2.y;
		retVec.y = v1.z * v2.x - v1.x * v2.z;
		retVec.z = v1.x * v2.y - v1.y * v2.x;
		return retVec;
	}

	template <typename T>
	inline T dot_vec3(vec3<T>& v1, vec3<T>& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	// interpolation ===================================================================

	template <typename T>
	inline vec3<T> mix_vec3(vec3<T>& v1, vec3<T>& v2, float u)
	{
		vec3<T> retVec;
		retVec.x = v1.x * (1 - u) + v2.x * u;
		retVec.y = v1.y * (1 - u) + v2.y * u;
		retVec.z = v1.z * (1 - u) + v2.z * u;
		return retVec;
	}

	
	// vec4 transform functions ========================================================================

	template <typename T>
	inline vec4<T> convert_vec4(T* data)
	{
		assert(data != nullptr);

		vec4<T> vec;
		vec.x = *data;
		data += sizeof(T);
		vec.y = *data;
		data += sizeof(T);
		vec.z = *data;
		data += sizeof(T);
		vec.w = *data;
		return vec;
	}

	template <typename T>
	inline T length_vec4(vec4<T>& v4)
	{
		return std::sqrt(v4.x * v4.x + v4.y * v4.y + v4.z * v4.z + v4.w * v4.w);
	}

	template <typename T>
	inline vec4<T> normalise_vec4(vec4<T>& v4)
	{
		vec4<T> retVec;
		T length = length_vec4(v4);
		T invLength = static_cast<T>(1) / length;

		retVec.x = v4.x * invLength;
		retVec.y = v4.y * invLength;
		retVec.z = v4.z * invLength;
		retVec.w = v4.w * invLength;
		return retVec;
	}

	// interpolation =========================================================

	template <typename T>
	inline vec4<T> mix_vec4(vec4<T>& v1, vec4<T>& v2, float u)
	{
		vec4<T> retVec;
		retVec.x = v1.x * (1 - u) + v2.x * u;
		retVec.y = v1.y * (1 - u) + v2.y * u;
		retVec.z = v1.z * (1 - u) + v2.z * u;
		retVec.w = v1.w * (1 - u) + v2.w * u;
		return retVec;
	}

	// matrix conversion  ===============================================================================

	template <typename T>
	inline mat4<T> convert_mat4(T* data)
	{
		assert(data != nullptr);

		mat4<T> mat;

		for (uint8_t row = 0; row < 4; ++row) {

			vec4<T> vec;
			vec.x = *data;
			data += sizeof(T);
			vec.y = *data;
			data += sizeof(T);
			vec.z = *data;
			data += sizeof(T);
			vec.w = *data;

			mat(vec, row);
		}
		return mat;
	}


	// matrix TRS ========================================================================================
	template <typename T>
	inline mat4<T> translate(mat4<T>& mat, vec3<T>& trans)
	{
		mat4<T> retMat = mat;

		retMat(0, 3) = trans.x;
		retMat(1, 3) = trans.y;
		retMat(2, 3) = trans.x;
		retMat(3, 3) = 1.0f;

		return retMat;
	}

	template <typename T>
	inline mat4<T> translate(mat4<T>& mat, vec4<T>& trans)
	{
		mat4<T> retMat = mat;

		retMat(0, 3) = trans.x;
		retMat(1, 3) = trans.y;
		retMat(2, 3) = trans.x;
		retMat(3, 3) = 1.0f;

		return retMat;
	}

	template <typename T>
	inline mat4<T> scale(mat4<T>& mat, vec3<T>& scale)
	{
		mat4<T> retMat = mat;

		retMat(0, 0) = scale.x;
		retMat(1, 1) = scale.y;
		retMat(2, 2) = scale.x;
		retMat(3, 3) = 1.0f;
		return retMat;
	}

	template <typename T>
	inline mat4<T> lookAt(vec3<T>& position, vec3<T>& target, vec3<T>& up_vec)
	{
		vec3<T> dir = normalise_vec3(position - target);
		vec3<T> right = cross_vec3(up_vec, dir);
		right = normalise_vec3(right);
		vec3<T> cam_up = cross_vec3(dir, right);

		// create the output lookat matrix
		mat4<T> lookAt;
		lookAt(0, 0) = right.x;
		lookAt(0, 1) = right.y;
		lookAt(0, 2) = right.z;
		lookAt(1, 0) = cam_up.x;
		lookAt(1, 1) = cam_up.y;
		lookAt(1, 2) = cam_up.z;
		lookAt(2, 0) = dir.x;
		lookAt(2, 1) = dir.y;
		lookAt(2, 2) = dir.z;

		mat4<T> posMat;
		lookAt(0, 3) = -position.x;
		lookAt(1, 3) = -position.y;
		lookAt(2, 3) = -position.z;
		lookAt = lookAt * posMat;

		return lookAt;
	}

	template <typename T>
	inline mat4<T> perspective(T fov, T aspect, T zNear, T zFar)
	{
		float t = std::tan(fov * 0.5f * M_PI / 180) * zNear;
		float r = aspect * t;
		float l = -r;
		float b = -t;

		mat4<T> retMat;
		retMat(0, 0) = 2 * zNear / (r - l);
		retMat(1, 1) = 2 * zNear / (t - b);
		retMat(2, 0) = (r + l) / (r - l);
		retMat(2, 1) = (t + b) / (t - b);
		retMat(2, 2) = -(zFar + zNear) / (zFar - zNear);
		retMat(2, 3) = -1;
		retMat(3, 2) = -2 * zFar * zNear / (zFar - zNear);
		return retMat;
	}
}
