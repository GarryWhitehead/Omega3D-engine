#pragma once

#include <assert.h>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include "OEMaths.h"

namespace OEMaths
{

	// vectors
	vec2f convert_vec2_F(const float* data);
	vec2f convert_vec2_D(const double* data);
	
	vec3f convert_vec3_F(const float* data);
	vec3f convert_vec3_D(const double* data);

	vec4f vec3_to_vec4(vec3f v3, float w);

	float length_vec3(vec3f& v3);
	vec3f normalise_vec3(vec3f& v3);
	vec3f cross_vec3(vec3f& v1, vec3f& v2);
	float dot_vec3(vec3f& v1, vec3f& v2);
	vec3f mix_vec3(vec3f& v1, vec3f& v2, float u);

	vec4f convert_vec4_F(const float* data);
	vec4f convert_vec4_D(const double* data);
	float length_vec4(vec4f& v4);
	vec4f normalise_vec4(vec4f& v4);
	vec4f mix_vec4(vec4f& v1, vec4f& v2, float u);

	// matrices
	mat4f convert_mat4(void* data);

	mat4f translate_mat4(vec3f& trans);
	mat4f scale_mat4(vec3f& scale);

	mat4f rotate_mat4(float theta, vec3f& axis);
	mat4f lookAt(vec3f& position, vec3f& target, vec3f& up_vec);
	mat4f orthoProjection(float zoom, float aspect, float zNear, float zFar);
	mat4f perspective(float fov, float aspect, float zNear, float zFar);
	mat4f ortho(float left, float right, float top, float bottom, float zNear, float zFar);
	mat4f mat4_inverse(mat4f& m);

	
}
