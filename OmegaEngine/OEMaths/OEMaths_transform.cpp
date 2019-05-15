#include "OEMaths_transform.h"
#include "OEMaths/OEMaths_Mat4.h"
#include "OEMaths/OEMaths_Vec3.h"

namespace OEMaths
{
	mat4f lookAt(vec3f& position, vec3f& target, vec3f& up_vec)
	{
		vec3f dir = target - position;
		dir.normalise();
		vec3f right = up_vec.cross(dir);
		right.normalise();
		vec3f cam_up = dir.cross(right);
		cam_up.normalise();

		// create the output lookat matrix
		mat4f result;
		result(0, 0) = right.getX();
		result(0, 1) = right.getY();
		result(0, 2) = right.getZ();

		result(1, 0) = cam_up.getX();
		result(1, 1) = cam_up.getY();
		result(1, 2) = cam_up.getZ();

		result(2, 0) = dir.getX();
		result(2, 1) = dir.getY();
		result(2, 2) = dir.getZ();

		result(3, 0) = -right.dot(position);
		result(3, 1) = -cam_up.dot(position);
		result(3, 2) = -dir.dot(position);

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
		float rad_fov = fov * (float)M_PI / 180.0f;
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

}