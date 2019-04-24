#pragma once

#include <assert.h>
#include <cstdint>

#include "OEMaths.h"

namespace OEMaths
{
	// vec2 conversion ================================================================

	template <typename T>
	inline vec2<T> convert_vec2(const void* data)
	{
		assert(data != nullptr);
		T* ptr = (T*)data;
		vec2<T> vec;
		vec.x = *ptr;
		++ptr;
		vec.y = *ptr;

		return vec;
	}
	
	// vec3 transform functions ========================================================================================================
	
	template <typename T>
	inline vec3<T> convert_vec3(const void* data)
	{
		assert(data != nullptr);
		T* ptr = (T*)data;
		vec3<T> vec;
		vec.x = *ptr;
		++ptr;
		vec.y = *ptr;
		++ptr;
		vec.z = *ptr;
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

		retVec.x = v3.x / length;
		retVec.y = v3.y / length;
		retVec.z = v3.z / length;
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
		retVec.x = v1.x * (T(1) - u) + v2.x * u;
		retVec.y = v1.y * (T(1) - u) + v2.y * u;
		retVec.z = v1.z * (T(1) - u) + v2.z * u;
		return retVec;
	}

	
	// vec4 transform functions ========================================================================

	template <typename T>
	inline vec4<T> convert_vec4(const void* data)
	{
		assert(data != nullptr);
		T* ptr = (T*)data;
		vec4<T> vec;
		vec.x = *ptr;
		++ptr;
		vec.y = *ptr;
		++ptr;
		vec.z = *ptr;
		++ptr;
		vec.w = *ptr;
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

		retVec.x = v4.x / length;
		retVec.y = v4.y / length;
		retVec.z = v4.z / length;
		retVec.w = v4.w / length;
		return retVec;
	}

	// interpolation =========================================================

	template <typename T>
	inline vec4<T> mix_vec4(vec4<T>& v1, vec4<T>& v2, float u)
	{
		vec4<T> retVec;
		retVec.x = v1.x * (T(1) - u) + v2.x * u;
		retVec.y = v1.y * (T(1) - u) + v2.y * u;
		retVec.z = v1.z * (T(1) - u) + v2.z * u;
		retVec.w = v1.w * (T(1) - u) + v2.w * u;
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
	inline mat4<T> translate_mat4(vec3<T>& trans)
	{
		mat4<T> retMat;
		retMat(3, 0) = trans.x;
		retMat(3, 1) = trans.y;
		retMat(3, 2) = trans.x;
		retMat(3, 3) = T(1);

		return retMat;
	}

	template <typename T>
	inline mat4<T> scale_mat4(vec3<T>& scale)
	{
		mat4<T> result;
		result(0, 0) = scale.x;
		result(1, 1) = scale.y;
		result(2, 2) = scale.z;
		result(3, 3) = T(1);
		return result;
	}

	template <typename T>
	inline mat4<T> rotate_mat4(T theta, vec3<T> axis)
	{
		mat4<T> retMat;
		vec3<T> axis_norm = axis / (theta == T(0) ? T(1) : theta);	//avoid divide by zero
		T xy = axis_norm.x * axis_norm.y;
		T yz = axis_norm.y * axis_norm.z;
		T zx = axis_norm.z * axis_norm.x;

		T cosTheta = std::cos(theta);
		T sinTheta = std::sin(theta);
		
		retMat(0, 0) = cosTheta + axis_norm.x * axis_norm.x * (T(1) - cosTheta);
		retMat(0, 1) = xy * (T(1) - cosTheta) - axis_norm.z * sinTheta;
		retMat(0, 2) = zx * (T(1) - cosTheta) + axis_norm.y * sinTheta;

		retMat(1, 0) = xy * (T(1) - cosTheta) + axis_norm.z * sinTheta;
		retMat(1, 1) = cosTheta + axis_norm.y * axis_norm.y * (T(1) - cosTheta);
		retMat(1, 2) = yz * (T(1) - cosTheta) - axis_norm.x * sinTheta;

		retMat(2, 0) = zx * (T(1) - cosTheta) - axis_norm.y * sinTheta;
		retMat(2, 1) = yz * (T(1) - cosTheta) + axis_norm.x * sinTheta;
		retMat(2, 2) = cosTheta + axis_norm.z * axis_norm.z * (T(1) - cosTheta);

		return retMat;
	}

	template <typename T>
	inline mat4<T> lookAt(vec3<T>& position, vec3<T>& target, vec3<T>& up_vec)
	{
		vec3<T> dir = normalise_vec3(target - position);
		vec3<T> right = normalise_vec3(cross_vec3(up_vec, dir));
		vec3<T> cam_up = normalise_vec3(cross_vec3(dir, right));

		// create the output lookat matrix
		mat4<T> result;
		result(0, 0) = right.x;
		result(0, 1) = right.y;
		result(0, 2) = right.z;
		
		result(1, 0) = cam_up.x;
		result(1, 1) = cam_up.y;
		result(1, 2) = cam_up.z;
		
		result(2, 0) = dir.x;
		result(2, 1) = dir.y;
		result(2, 2) = dir.z;
		
		result(3, 0) = -dot_vec3(right, position);
		result(3, 1) = -dot_vec3(cam_up, position);
		result(3, 2) = -dot_vec3(dir, position);

		return result;
	}

	template <typename T>
	inline mat4<T> orthoProjection(T zoom, T aspect, T zNear, T zFar)
	{
		mat4<T> result;
		result(0, 0) = zoom / aspect;
		result(1, 1) = -zoom;
		result(2, 2) = T(2) / (zFar - zNear);
		result(2, 3) = -(zFar + zNear) / (zFar - zNear);

		return result;
	}

	template <typename T>
	inline mat4<T> perspective(T fov, T aspect, T zNear, T zFar)
	{
		// fov to radians
		float rad_fov = fov * M_PI / T(180);
		float tanHalfFov = std::tan(rad_fov * T(0.5));
	
		mat4<T> result;
		result(0, 0) = T(1) / (aspect * tanHalfFov);
		
		result(1, 1) = T(1) / tanHalfFov;

		result(2, 2) = zFar / (zFar - zNear);		// note: this is Vulkan specific i.e. using 0-1 for depth
		result(2, 3) = T(1);

		result(3, 2) = -(zFar * zNear) / (zFar - zNear);
		result(3, 3) = T(0);

		return result;
	}

	template <typename T>
	inline mat4<T> ortho(T left, T right, T top, T bottom, T zNear, T zFar)
	{
		mat4<T> result;
		result(0, 0) = T(2) / (right - left);
		result(1, 1) = T(2) / (top - bottom);
		result(2, 2) = T(2) / (zFar - zNear);
		result(3, 0) = -(right + left) / (right - left);
		result(3, 1) = -(top + bottom) / (top - bottom);
		result(3, 2) = -(zFar + zNear) / (zFar - zNear);
		return result;
	}


	template <typename T>
	inline mat4<T> mat4_inverse(mat4<T>& m)
	{
		mat4<T> inv;
		mat4<T> result;
		mat4<T> det;

		inv[0] = m[5]  * m[10] * m[15] - 
				m[5]  * m[11] * m[14] - 
				m[9]  * m[6]  * m[15] + 
				m[9]  * m[7]  * m[14] +
				m[13] * m[6]  * m[11] - 
				m[13] * m[7]  * m[10];

		inv[4] = -m[4]  * m[10] * m[15] + 
				m[4]  * m[11] * m[14] + 
				m[8]  * m[6]  * m[15] - 
				m[8]  * m[7]  * m[14] - 
				m[12] * m[6]  * m[11] + 
				m[12] * m[7]  * m[10];

		inv[8] = m[4]  * m[9] * m[15] - 
				m[4]  * m[11] * m[13] - 
				m[8]  * m[5] * m[15] + 
				m[8]  * m[7] * m[13] + 
				m[12] * m[5] * m[11] - 
				m[12] * m[7] * m[9];

		inv[12] = -m[4]  * m[9] * m[14] + 
				m[4]  * m[10] * m[13] +
				m[8]  * m[5] * m[14] - 
				m[8]  * m[6] * m[13] - 
				m[12] * m[5] * m[10] + 
				m[12] * m[6] * m[9];

		inv[1] = -m[1]  * m[10] * m[15] + 
				m[1]  * m[11] * m[14] + 
				m[9]  * m[2] * m[15] - 
				m[9]  * m[3] * m[14] - 
				m[13] * m[2] * m[11] + 
				m[13] * m[3] * m[10];

		inv[5] = m[0]  * m[10] * m[15] - 
				m[0]  * m[11] * m[14] - 
				m[8]  * m[2] * m[15] + 
				m[8]  * m[3] * m[14] + 
				m[12] * m[2] * m[11] - 
				m[12] * m[3] * m[10];

		inv[9] = -m[0]  * m[9] * m[15] + 
				m[0]  * m[11] * m[13] + 
				m[8]  * m[1] * m[15] - 
				m[8]  * m[3] * m[13] - 
				m[12] * m[1] * m[11] + 
				m[12] * m[3] * m[9];

		inv[13] = m[0]  * m[9] * m[14] - 
				m[0]  * m[10] * m[13] - 
				m[8]  * m[1] * m[14] + 
				m[8]  * m[2] * m[13] + 
				m[12] * m[1] * m[10] - 
				m[12] * m[2] * m[9];

		inv[2] = m[1]  * m[6] * m[15] - 
				m[1]  * m[7] * m[14] - 
				m[5]  * m[2] * m[15] + 
				m[5]  * m[3] * m[14] + 
				m[13] * m[2] * m[7] - 
				m[13] * m[3] * m[6];

		inv[6] = -m[0]  * m[6] * m[15] + 
				m[0]  * m[7] * m[14] + 
				m[4]  * m[2] * m[15] - 
				m[4]  * m[3] * m[14] - 
				m[12] * m[2] * m[7] + 
				m[12] * m[3] * m[6];

		inv[10] = m[0]  * m[5] * m[15] - 
				m[0]  * m[7] * m[13] - 
				m[4]  * m[1] * m[15] + 
				m[4]  * m[3] * m[13] + 
				m[12] * m[1] * m[7] - 
				m[12] * m[3] * m[5];

		inv[14] = -m[0]  * m[5] * m[14] + 
				m[0]  * m[6] * m[13] + 
				m[4]  * m[1] * m[14] - 
				m[4]  * m[2] * m[13] - 
				m[12] * m[1] * m[6] + 
				m[12] * m[2] * m[5];

		inv[3] = -m[1] * m[6] * m[11] + 
				m[1] * m[7] * m[10] + 
				m[5] * m[2] * m[11] - 
				m[5] * m[3] * m[10] - 
				m[9] * m[2] * m[7] + 
				m[9] * m[3] * m[6];

		inv[7] = m[0] * m[6] * m[11] - 
				m[0] * m[7] * m[10] - 
				m[4] * m[2] * m[11] + 
				m[4] * m[3] * m[10] + 
				m[8] * m[2] * m[7] - 
				m[8] * m[3] * m[6];

		inv[11] = -m[0] * m[5] * m[11] + 
				m[0] * m[7] * m[9] + 
				m[4] * m[1] * m[11] - 
				m[4] * m[3] * m[9] - 
				m[8] * m[1] * m[7] + 
				m[8] * m[3] * m[5];

		inv[15] = m[0] * m[5] * m[10] - 
				m[0] * m[6] * m[9] - 
				m[4] * m[1] * m[10] + 
				m[4] * m[2] * m[9] + 
				m[8] * m[1] * m[6] - 
				m[8] * m[2] * m[5];

		det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

		if (det == 0) {
			// just return a identity matrix
			return result;
		}

		det = 1.0 / det;

		for (uint32_t i = 0; i < 16; i++) {
			result[i] = inv[i] * det;
		}

		return result;
	}

	
}
