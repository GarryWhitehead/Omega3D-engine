#pragma once

#include <assert.h>
#include <cstdint>
#include <algorithm>
#include <cmath>

#include "OEMaths.h"

namespace OEMaths
{


	// matrices
	mat4f convert_mat4_F(const float* data);
	mat4f convert_mat4_D(const double* data);

	mat4f translate_mat4(vec3f& trans);
	mat4f scale_mat4(vec3f& scale);

	mat4f rotate_mat4(float theta, vec3f& axis);
	mat4f lookAt(vec3f& position, vec3f& target, vec3f& up_vec);
	mat4f orthoProjection(float zoom, float aspect, float zNear, float zFar);
	mat4f perspective(float fov, float aspect, float zNear, float zFar);
	mat4f ortho(float left, float right, float top, float bottom, float zNear, float zFar);
	mat4f mat4_inverse(mat4f& m);

	
}
