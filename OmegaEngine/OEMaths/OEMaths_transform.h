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
	inline vec3<T> convert_vec3(void* data)
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
		T invLength = static_cast<T>(1) / length == 0 ? 1 : length;

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
		++data;
		vec.y = *data;
		++data;
		vec.z = *data;
		++data;
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
		T invLength = static_cast<T>(1) / length == 0 ? 1 : length;

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
	inline mat4<T> translate_mat4(vec3<T>& trans)
	{
		mat4<T> retMat;
		retMat(0, 3) = trans.x;
		retMat(1, 3) = trans.y;
		retMat(2, 3) = trans.x;
		retMat(3, 3) = 1.0f;

		return retMat;
	}

	template <typename T>
	inline mat4<T> scale_mat4(vec3<T>& scale)
	{
		mat4<T> result;
		result(0, 0) = scale.x;
		result(1, 1) = scale.y;
		result(2, 2) = scale.z;
		result(3, 3) = 1.0f;
		return result;
	}

	template <typename T>
	inline mat4<T> rotate_mat4(T theta, vec3<T> axis)
	{
		mat4<T> retMat;
		vec3<T> axis_norm = axis / (theta == 0 ? 1 : theta);	//avoid divide by zero
		T xy = axis_norm.x * axis_norm.y;
		T yz = axis_norm.y * axis_norm.z;
		T zx = axis_norm.z * axis_norm.x;

		T cosTheta = std::cos(theta);
		T sinTheta = std::sin(theta);
		
		retMat(0, 0) = cosTheta + axis_norm.x * axis_norm.x * (1 - cosTheta);
		retMat(0, 1) = xy * (1 - cosTheta) - axis_norm.z * sinTheta;
		retMat(0, 2) = zx * (1 - cosTheta) + axis_norm.y * sinTheta;

		retMat(1, 0) = xy * (1 - cosTheta) + axis_norm.z * sinTheta;
		retMat(1, 1) = cosTheta + axis_norm.y * axis_norm.y * (1 - cosTheta);
		retMat(1, 2) = yz * (1 - cosTheta) - axis_norm.x * sinTheta;

		retMat(2, 0) = zx * (1 - cosTheta) - axis_norm.y * sinTheta;
		retMat(2, 1) = yz * (1 - cosTheta) + axis_norm.x * sinTheta;
		retMat(2, 2) = cosTheta + axis_norm.z * axis_norm.z * (1 - cosTheta);

		return retMat;
	}

	template <typename T>
	inline mat4<T> lookAt(vec3<T>& position, vec3<T>& target, vec3<T>& up_vec)
	{
		vec3<T> dir = normalise_vec3(position - target);
		vec3<T> right = normalise_vec3(cross_vec3(up_vec, dir));
		vec3<T> cam_up = normalise_vec3(cross_vec3(dir, right));

		// create the output lookat matrix
		mat4<T> lookAt;
		lookAt(0, 0) = right.x;
		lookAt(0, 1) = right.y;
		lookAt(0, 2) = right.z;
		lookAt(0, 3) = -dot_vec3(right, position);
		lookAt(1, 0) = cam_up.x;
		lookAt(1, 1) = cam_up.y;
		lookAt(1, 2) = cam_up.z;
		lookAt(1, 3) = -dot_vec3(cam_up, position);
		lookAt(2, 0) = dir.x;
		lookAt(2, 1) = dir.y;
		lookAt(2, 2) = dir.z;
		lookAt(2, 3) = -dot_vec3(dir, position);

		return lookAt;
	}

	template <typename T>
	inline mat4<T> orthoProjection(T zoom, T aspect, T zNear, T zFar)
	{
		mat4<T> result;
		result(0, 0) = zoom / aspect;
		result(1, 1) = -zoom;
		result(2, 2) = 2 / (zFar - zNear);
		result(2, 3) = -(zFar + zNear) / (zFar - zNear);

		return result;
	}

	template <typename T>
	inline mat4<T> perspective(T fov, T aspect, T zNear, T zFar)
	{
		// fov to radians
		float rad_fov = fov * M_PI / 180;
		float tanHalfFov = std::tan(rad_fov * 0.5f);
	
		mat4<T> result;
		result(0, 0) = 1 / (aspect * tanHalfFov);

		result(1, 1) = 1 / tanHalfFov;

		result(2, 2) = (zFar + zNear) / (zFar - zNear);
		result(2, 3) = -(2 * zFar * zNear) / (zFar - zNear);

		result(3, 0) = 0;
		result(3, 1) = 0;
		result(3, 2) = 1;
		result(3, 3) = 0;
		return result;
	}

	template <typename T>
	inline mat4<T> ortho(T left, T right, T top, T bottom, T zNear, T zFar)
	{
		mat4<T> result;
		result(0, 0) = 2 / (right - left);
		result(1, 1) = 2 / (top - bottom);
		result(2, 2) = 2 / (zFar - zNear);
		result(3, 0) = -(right + left) / (right - left);
		result(3, 1) = -(top + bottom) / (top - bottom);
		result(3, 2) = -(zFar + zNear) / (zFar - zNear);
		return result;
	}

	template <typename T>
	inline T mat4_det(mat4<T>& mat)
	{
		return mat[0] * (mat[5] * (mat[10] * mat[15] - mat[11] * mat[14])
		+ mat[6] * (mat[11] * mat[13] - mat[9] * mat[15])
		+ mat[7] * (mat[9] * mat[14] - mat[10] * mat[13]))

		+ mat[1] * (mat[4] * (mat[11] * mat[14] - mat[10] * mat[15])
		+ mat[6] * (mat[8] * mat[15] - mat[11] * mat[12])
		+ mat[7] * (mat[10] * mat[12] - mat[8] * mat[14]))

		+ mat[2] * (mat[4] * (mat[9] * mat[15] - mat[11] * mat[13])
		+ mat[5] * (mat[11] * mat[12] - mat[8] * mat[15])
		+ mat[7] * (mat[8] * mat[13] - mat[9] * mat[12]))

		+ mat[3] * (mat[4] * (mat[10] * mat[13] - mat[9] * mat[14])
		+ mat[5] * (mat[8] * mat[14] - mat[10] * mat[12])
		+ mat[6] * (mat[9] * mat[12] - mat[8] * mat[13]));
		
	}

	template <typename T>
	inline mat4<T> mat4_inverse(mat4<T>& mat)
	{
		mat4<T> retMat;
		T det = mat4_det<T>(mat);

		if (std::fabs(det) > EPSILON) {

			T m4m9 = mat[4] * mat[9];
			T m4m10 = mat[4] * mat[10];
			T m4m11 = mat[4] * mat[11];
			T m4m13 = mat[4] * mat[13];
			T m4m14 = mat[4] * mat[14];
			T m4m15 = mat[4] * mat[15];

			T m5m8 = mat[5] * mat[8];
			T m5m10 = mat[5] * mat[10];
			T m5m11 = mat[5] * mat[11];
			T m5m12 = mat[5] * mat[12];
			T m5m14 = mat[5] * mat[14];
			T m5m15 = mat[5] * mat[15];

			T m6m8 = mat[6] * mat[8];
			T m6m9 = mat[6] * mat[9];
			T m6m11 = mat[6] * mat[11];
			T m6m12 = mat[6] * mat[12];
			T m6m13 = mat[6] * mat[13];
			T m6m15 = mat[6] * mat[15];

			T m7m8 = mat[7] * mat[8];
			T m7m9 = mat[7] * mat[9];
			T m7m10 = mat[7] * mat[10];
			T m7m12 = mat[7] * mat[12];
			T m7m13 = mat[7] * mat[13];
			T m7m14 = mat[7] * mat[14];

			T m8m13 = mat[8] * mat[13];
			T m8m14 = mat[8] * mat[14];
			T m8m15 = mat[8] * mat[15];

			T m9m12 = mat[9] * mat[12];
			T m9m14 = mat[9] * mat[14];
			T m9m15 = mat[9] * mat[15];

			T m10m12 = mat[10] * mat[12];
			T m10m13 = mat[10] * mat[13];
			T m10m15 = mat[10] * mat[15];

			T m11m12 = mat[11] * mat[12];
			T m11m13 = mat[11] * mat[13];
			T m11m14 = mat[11] * mat[14];


			retMat[0] = mat[5] * (m10m15 - m11m14) + mat[6] * (m11m13 - m9m15) + mat[7] * (m9m14 - m10m13);
			retMat[1] = mat[1] * (m11m14 - m10m15) + mat[2] * (m9m15 - m11m13) + mat[3] * (m10m13 - m9m14);
			retMat[2] = mat[1] * (m6m15 - m7m14) + mat[2] * (m7m13 - m5m15) + mat[3] * (m5m14 - m6m13);
			retMat[3] = mat[1] * (m7m10 - m6m11) + mat[2] * (m5m11 - m7m9) + mat[3] * (m6m9 - m5m10);

			retMat[4] = mat[4] * (m11m14 - m10m15) + mat[6] * (m8m15 - m11m12) + mat[7] * (m10m12 - m8m14);
			retMat[5] = mat[0] * (m10m15 - m11m14) + mat[2] * (m11m12 - m8m15) + mat[3] * (m8m14 - m10m12);
			retMat[6] = mat[0] * (m7m14 - m6m15) + mat[2] * (m4m15 - m7m12) + mat[3] * (m6m12 - m4m14);
			retMat[7] = mat[0] * (m6m11 - m7m10) + mat[2] * (m7m8 - m4m11) + mat[3] * (m4m10 - m6m8);

			retMat[8] = mat[4] * (m9m15 - m11m13) + mat[5] * (m11m12 - m8m15) + mat[7] * (m8m13 - m9m12);
			retMat[9] = mat[0] * (m11m13 - m9m15) + mat[1] * (m8m15 - m11m12) + mat[3] * (m9m12 - m8m13);
			retMat[10] = mat[0] * (m5m15 - m7m13) + mat[1] * (m7m12 - m4m15) + mat[3] * (m4m13 - m5m12);
			retMat[11] = mat[0] * (m7m9 - m5m11) + mat[1] * (m4m11 - m7m8) + mat[3] * (m5m8 - m4m9);

			retMat[12] = mat[4] * (m10m13 - m9m14) + mat[5] * (m8m14 - m10m12) + mat[6] * (m9m12 - m8m13);
			retMat[13] = mat[0] * (m9m14 - m10m13) + mat[1] * (m10m12 - m8m14) + mat[2] * (m8m13 - m9m12);
			retMat[14] = mat[0] * (m6m13 - m5m14) + mat[1] * (m4m14 - m6m12) + mat[2] * (m5m12 - m4m13);
			retMat[15] = mat[0] * (m5m10 - m6m9) + mat[1] * (m6m8 - m4m10) + mat[2] * (m4m9 - m5m8);

			retMat /= det;
		}

		return retMat;
	}
}
