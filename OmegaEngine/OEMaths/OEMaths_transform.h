#pragma once

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <cstdint>

#include "OEMaths.h"

namespace OEMaths
{
class mat4f;
class vec3f;

mat4f lookAt(vec3f &position, vec3f &target, vec3f &up_vec);
mat4f orthoProjection(float zoom, float aspect, float zNear, float zFar);
mat4f perspective(float fov, float aspect, float zNear, float zFar);
mat4f ortho(float left, float right, float top, float bottom, float zNear, float zFar);

} // namespace OEMaths
