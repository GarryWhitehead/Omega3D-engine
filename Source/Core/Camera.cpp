#include "Camera.h"

#include "OEMaths/OEMaths_transform.h"

#include <algorithm>

namespace OmegaEngine
{

void Camera::update()
{
	if (dir != MoveDirection::None)
	{
		//calculate the pitch and yaw vectors
		frontVec = OEMaths::vec3f{ std::cos(OEMaths::radians(rotation.x)) * std::cos(OEMaths::radians(rotation.y)),
			                       std::sin(OEMaths::radians(rotation.x)),
			                       std::cos(OEMaths::radians(rotation.x)) * std::sin(OEMaths::radians(rotation.y)) };
		frontVec = OEMaths::normalise(frontVec);

		if (dir == MoveDirection::Up)
		{
			position += frontVec * velocity;    // Engine::DT;	<- Needed?
		}
		if (dir == MoveDirection::Down)
		{
			position -= frontVec * velocity;    // Engine::DT;
		}
		if (dir == MoveDirection::Left)
		{
			OEMaths::vec3f cameraRight = OEMaths::normalise(OEMaths::vec3f::cross(frontVec, cameraUp));
			position -= cameraRight * velocity;
		}
		if (dir == MoveDirection::Right)
		{
			OEMaths::vec3f cameraRight = OEMaths::normalise(OEMaths::vec3f::cross(frontVec, cameraUp));
			position += cameraRight * velocity;
		}

		updateViewMatrix();
	}
}

void Camera::updateViewMatrix()
{
	OEMaths::vec3f target = position + frontVec;
	currentView = OEMaths::lookAt(position, target, cameraUp);
}

void Camera::rotate(float dx, float dy)
{
}

void Camera::translate(float dx, float dy, float dz)
{
}

void Camera::updateDirection(const Camera::MoveDirection moveDir)
{
	dir = moveDir;
}

// =================== getters ===========================

OEMaths::mat4f Camera::getMvpMatrix()
{
	return currentProj * currentView * currentModel;
}

float Camera::getZNear() const
{
	return zNear;
}

float Camera::getZFar() const
{
	return zFar;
}

float Camera::getFov() const
{
	return fov;
}

OEMaths::vec3f& Camera::getPos()
{
	return position;
}

OEMaths::mat4f& Camera::getProjMatrix()
{
	return currentProj;
}

OEMaths::mat4f& Camera::getViewMatrix()
{
	return currentView;
}

OEMaths::mat4f& Camera::getModelMatrix()
{
	return currentModel;
}

// ========================= setters =============================
void Camera::setPerspective()
{
	currentProj = OEMaths::perspective(fov, aspect, zNear, zFar);
}

void Camera::setFov(const float camFov)
{
	fov = camFov;
}

void Camera::setZNear(const float zn)
{
	zNear = zn;
}

void Camera::setZFar(const float zf)
{
	zFar = zf;
}

void Camera::setAspect(const float asp)
{
	aspect = asp;
}

void Camera::setVelocity(const float vel)
{
	velocity = vel;
}

void Camera::setType(const CameraType camType)
{
	type = camType;
}

void Camera::setPosition(const OEMaths::vec3f& pos)
{
	position = pos;
}

}    // namespace OmegaEngine
