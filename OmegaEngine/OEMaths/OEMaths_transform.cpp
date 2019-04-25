#include "OEMaths_transform.h"

namespace OEMaths
{
	// vec2 conversion ================================================================

	vec2f convert_vec2_F(const float* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;
		vec2f vec;
		vec.x = *ptr;
		++ptr;
		vec.y = *ptr;

		return vec;
	}

	vec2f convert_vec2_D(const double* data)
	{
		assert(data != nullptr);
		double* ptr = (double*)data;
		vec2f vec;
		vec.x = (float)*ptr;
		++ptr;
		vec.y = (float)*ptr;

		return vec;
	}

	// vec3 transform functions ========================================================================================================

	vec3f convert_vec3_F(const float* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;
		vec3f vec;
		vec.x = *ptr;
		++ptr;
		vec.y = *ptr;
		++ptr;
		vec.z = *ptr;
		return vec;
	}

	vec3f convert_vec3_D(const double* data)
	{
		assert(data != nullptr);
		double* ptr = (double*)data;
		vec3f vec;
		vec.x = (float)*ptr;
		++ptr;
		vec.y = (float)*ptr;
		++ptr;
		vec.z = (float)*ptr;
		return vec;
	}


	// coversion from one vec type to another
	vec4f vec3_to_vec4(vec3f v3, float w)
	{
		vec4f v4;
		v4.x = v3.x;
		v4.y = v3.y;
		v4.z = v3.z;
		v4.w = w;
		return v4;
	}

	// Vector math functions ==============================================================

	float length_vec3(vec3f& v3)
	{
		return std::sqrt(v3.x * v3.x + v3.y * v3.y + v3.z * v3.z);
	}

	vec3f normalise_vec3(vec3f& v3)
	{
		vec3f retVec;
		float length = length_vec3(v3);

		retVec.x = v3.x / length;
		retVec.y = v3.y / length;
		retVec.z = v3.z / length;
		return retVec;
	}

	vec3f cross_vec3(vec3f& v1, vec3f& v2)
	{
		vec3f retVec;
		retVec.x = v1.y * v2.z - v1.z * v2.y;
		retVec.y = v1.z * v2.x - v1.x * v2.z;
		retVec.z = v1.x * v2.y - v1.y * v2.x;
		return retVec;
	}

	float dot_vec3(vec3f& v1, vec3f& v2)
	{
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	// interpolation ===================================================================

	vec3f mix_vec3(vec3f& v1, vec3f& v2, float u)
	{
		vec3f retVec;
		retVec.x = v1.x * (1.0f - u) + v2.x * u;
		retVec.y = v1.y * (1.0f - u) + v2.y * u;
		retVec.z = v1.z * (1.0f - u) + v2.z * u;
		return retVec;
	}


	// vec4 transform functions ========================================================================

	vec4f convert_vec4_F(const float* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;
		vec4f vec;
		vec.x = *ptr;
		++ptr;
		vec.y = *ptr;
		++ptr;
		vec.z = *ptr;
		++ptr;
		vec.w = *ptr;
		return vec;
	}

	vec4f convert_vec4_D(const double* data)
	{
		assert(data != nullptr);
		double* ptr = (double*)data;
		vec4f vec;
		vec.x = (float)*ptr;
		++ptr;
		vec.y = (float)*ptr;
		++ptr;
		vec.z = (float)*ptr;
		++ptr;
		vec.w = (float)*ptr;
		return vec;
	}

	float length_vec4(vec4f& v4)
	{
		return std::sqrt(v4.x * v4.x + v4.y * v4.y + v4.z * v4.z + v4.w * v4.w);
	}

	vec4f normalise_vec4(vec4f& v4)
	{
		vec4f retVec;
		float length = length_vec4(v4);

		retVec.x = v4.x / length;
		retVec.y = v4.y / length;
		retVec.z = v4.z / length;
		retVec.w = v4.w / length;
		return retVec;
	}

	// interpolation =========================================================

	vec4f mix_vec4(vec4f& v1, vec4f& v2, float u)
	{
		vec4f retVec;
		retVec.x = v1.x * (1.0f - u) + v2.x * u;
		retVec.y = v1.y * (1.0f - u) + v2.y * u;
		retVec.z = v1.z * (1.0f - u) + v2.z * u;
		retVec.w = v1.w * (1.0f - u) + v2.w * u;
		return retVec;
	}

	// matrix conversion  ===============================================================================

	mat4f convert_mat4_F(const float* data)
	{
		assert(data != nullptr);

		mat4f mat;
		float* ptr = (float*)data;

		for (uint8_t col = 0; col < 4; ++col) {

			vec4f vec;
			vec.x = *ptr;
			++ptr;
			vec.y = *ptr;
			++ptr;
			vec.z = *ptr;
			++ptr;
			vec.w = *ptr;
			++ptr;

			mat(vec, col);
		}
		return mat;
	}

	mat4f convert_mat4_D(const double* data)
	{
		assert(data != nullptr);

		mat4f mat;
		double* ptr = (double*)data;

		for (uint8_t col = 0; col < 4; ++col) {

			vec4f vec;
			vec.x = (float)*ptr;
			++ptr;
			vec.y = (float)*ptr;
			++ptr;
			vec.z = (float)*ptr;
			++ptr;
			vec.w = (float)*ptr;
			++ptr;

			mat(vec, col);
		}
		return mat;
	}


	// matrix TRS ========================================================================================

	mat4f translate_mat4(vec3f& trans)
	{
		mat4f retMat;
		retMat(3, 0) = trans.x;
		retMat(3, 1) = trans.y;
		retMat(3, 2) = trans.x;
		retMat(3, 3) = 1.0f;

		return retMat;
	}

	mat4f scale_mat4(vec3f& scale)
	{
		mat4f result;
		result(0, 0) = scale.x;
		result(1, 1) = scale.y;
		result(2, 2) = scale.z;
		result(3, 3) = 1.0f;
		return result;
	}

	mat4f rotate_mat4(float theta, vec3f& axis)
	{
		mat4f retMat;
		vec3f axis_norm = axis / (theta == 0.0f ? 1.0f : theta);	//avoid divide by zero
		float xy = axis_norm.x * axis_norm.y;
		float yz = axis_norm.y * axis_norm.z;
		float zx = axis_norm.z * axis_norm.x;

		float cosTheta = std::cos(theta);
		float sinTheta = std::sin(theta);

		retMat(0, 0) = cosTheta + axis_norm.x * axis_norm.x * (1.0f - cosTheta);
		retMat(0, 1) = xy * (1.0f - cosTheta) - axis_norm.z * sinTheta;
		retMat(0, 2) = zx * (1.0f - cosTheta) + axis_norm.y * sinTheta;

		retMat(1, 0) = xy * (1.0f - cosTheta) + axis_norm.z * sinTheta;
		retMat(1, 1) = cosTheta + axis_norm.y * axis_norm.y * (1.0f - cosTheta);
		retMat(1, 2) = yz * (1.0f - cosTheta) - axis_norm.x * sinTheta;

		retMat(2, 0) = zx * (1.0f - cosTheta) - axis_norm.y * sinTheta;
		retMat(2, 1) = yz * (1.0f - cosTheta) + axis_norm.x * sinTheta;
		retMat(2, 2) = cosTheta + axis_norm.z * axis_norm.z * (1.0f - cosTheta);

		return retMat;
	}

	mat4f lookAt(vec3f& position, vec3f& target, vec3f& up_vec)
	{
		vec3f dir = normalise_vec3(target - position);
		vec3f right = normalise_vec3(cross_vec3(up_vec, dir));
		vec3f cam_up = normalise_vec3(cross_vec3(dir, right));

		// create the output lookat matrix
		mat4f result;
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

	mat4f orthoProjection(float zoom, float aspect, float zNear, float zFar)
	{
		mat4f result;
		result(0, 0) = zoom / aspect;
		result(1, 1) = -zoom;
		result(2, 2) = 2.0f / (zFar - zNear);
		result(2, 3) = -(zFar + zNear) / (zFar - zNear);

		return result;
	}

	mat4f perspective(float fov, float aspect, float zNear, float zFar)
	{
		// fov to radians
		float rad_fov = fov * M_PI / 180.0f;
		float tanHalfFov = std::tan(rad_fov * 0.5f);

		mat4f result;
		result(0, 0) = 1.0f / (aspect * tanHalfFov);

		result(1, 1) = 1.0f / tanHalfFov;

		result(2, 2) = zFar / (zFar - zNear);		// note: this is Vulkan specific i.e. using 0-1 for depth
		result(2, 3) = 1.0f;

		result(3, 2) = -(zFar * zNear) / (zFar - zNear);
		result(3, 3) = 0.0f;

		return result;
	}

	mat4f ortho(float left, float right, float top, float bottom, float zNear, float zFar)
	{
		mat4f result;
		result(0, 0) = 2.0f / (right - left);
		result(1, 1) = 2.0f / (top - bottom);
		result(2, 2) = 2.0f / (zFar - zNear);
		result(3, 0) = -(right + left) / (right - left);
		result(3, 1) = -(top + bottom) / (top - bottom);
		result(3, 2) = -(zFar + zNear) / (zFar - zNear);
		return result;
	}


	mat4f mat4_inverse(mat4f& m)
	{
		mat4f inv;
		mat4f result;
		float det;

		inv[0] = m[5] * m[10] * m[15] -
			m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +
			m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -
			m[13] * m[7] * m[10];

		inv[4] = -m[4] * m[10] * m[15] +
			m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -
			m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +
			m[12] * m[7] * m[10];

		inv[8] = m[4] * m[9] * m[15] -
			m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +
			m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -
			m[12] * m[7] * m[9];

		inv[12] = -m[4] * m[9] * m[14] +
			m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -
			m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +
			m[12] * m[6] * m[9];

		inv[1] = -m[1] * m[10] * m[15] +
			m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -
			m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +
			m[13] * m[3] * m[10];

		inv[5] = m[0] * m[10] * m[15] -
			m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +
			m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -
			m[12] * m[3] * m[10];

		inv[9] = -m[0] * m[9] * m[15] +
			m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -
			m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +
			m[12] * m[3] * m[9];

		inv[13] = m[0] * m[9] * m[14] -
			m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +
			m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -
			m[12] * m[2] * m[9];

		inv[2] = m[1] * m[6] * m[15] -
			m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +
			m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -
			m[13] * m[3] * m[6];

		inv[6] = -m[0] * m[6] * m[15] +
			m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -
			m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +
			m[12] * m[3] * m[6];

		inv[10] = m[0] * m[5] * m[15] -
			m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +
			m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -
			m[12] * m[3] * m[5];

		inv[14] = -m[0] * m[5] * m[14] +
			m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -
			m[4] * m[2] * m[13] -
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

		if (det == 0.0f) {
			// just return a identity matrix
			return result;
		}

		det = 1.0f / det;

		for (uint32_t i = 0; i < 16; i++) {
			result[i] = inv[i] * det;
		}

		return result;
	}


}