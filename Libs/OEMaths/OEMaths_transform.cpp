/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OEMaths_transform.h"
#include "OEMaths_Mat4.h"
#include "OEMaths_Vec3.h"

namespace OEMaths
{
mat4f lookAt(vec3f& position, vec3f& target, vec3f& upVec)
{
	vec3f dir = OEMaths::normalise(target - position);
	vec3f right = OEMaths::normalise(OEMaths::vec3f::cross(upVec, dir));
	vec3f camUp = OEMaths::normalise(OEMaths::vec3f::cross(dir, right));

	// create the output lookat matrix
	mat4f result;
	result[0] = { right, 0.0f };
	result[1] = { camUp, 0.0f };
	result[2] = { dir, 0.0f };
	result[3] = { -OEMaths::dot(right, position), -OEMaths::dot(camUp, position), -OEMaths::dot(dir, position), 1.0f };
	
	return result;
}

mat4f orthoProjection(float zoom, float aspect, float zNear, float zFar)
{
	mat4f result;
	result[0][0] = zoom / aspect;
	result[1][1] = -zoom;
	result[2][2] = 2.0f / (zFar - zNear);
	result[2][3] = -(zFar + zNear) / (zFar - zNear);

	return result;
}

mat4f perspective(float fov, float aspect, float zNear, float zFar)
{
	// fov to radians
	float rad_fov = fov * (float)M_PI / 180.0f;
	float tanHalfFov = std::tan(rad_fov * 0.5f);

	mat4f result;
	result[0][0] = 1.0f / (aspect * tanHalfFov);

	result[1][1] = 1.0f / tanHalfFov;

	result[2][2] = zFar / (zFar - zNear);    // note: this is Vulkan specific i.e. using 0-1 for depth
	result[2][3] = 1.0f;

	result[3][2] = -(zFar * zNear) / (zFar - zNear);
	result[3][3] = 0.0f;

#ifdef USE_VULKAN_COORDS
	result[1] *= -1.0f;

#endif

	return result;
}

mat4f ortho(float left, float right, float top, float bottom, float zNear, float zFar)
{
	mat4f result;
	result[0][0] = 2.0f / (right - left);
	result[1][1] = 2.0f / (top - bottom);
	result[2][2] = 2.0f / (zFar - zNear);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	result[3][2] = -(zFar + zNear) / (zFar - zNear);
	return result;
}

}    // namespace OEMaths
