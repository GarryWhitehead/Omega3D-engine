#include "Camera.h"

#include "OEMaths/OEMaths_transform.h"

#include <algorithm>

namespace OmegaEngine
{

void Camera::updatePosition(double xpos, double ypos)
{
	if (firstTime)
	{
		currentX = event.xpos;
		currentY = event.ypos;
		firstTime = false;
	}

	double deltaX = event.xpos - currentX;
	double deltaY = currentY - event.ypos;

	currentX = event.xpos;
	currentY = event.ypos;

	yaw -= deltaX * mouseSensitivity;
	pitch -= deltaY * mouseSensitivity;

	pitch = pitch > 89.0 ? 89.0 : pitch;
	pitch = pitch < -89.0 ? -89.0 : pitch;

	updateCameraRotation();
}

void Camera::update()
{
	if (dir != MoveDirection::None)
	{
		//calculate the pitch and yaw vectors
		frontVec = OEMaths::vec3f{ std::cos(OEMaths::radians(pitch)) * std::cos(OEMaths::radians(yaw)),
			                       std::sin(OEMaths::radians(pitch)),
			                       std::cos(OEMaths::radians(pitch)) * std::sin(OEMaths::radians(yaw)) };
		frontVec = OEMaths::normalise(frontVec);

		if (dir == MoveDirection::Up)
		{
			currentPos += frontVec * velocity;    // Engine::DT;	<- Needed?
		}
		if (dir == MoveDirection::Down)
		{
			currentPos -= frontVec * velocity;    // Engine::DT;
		}
		if (dir == MoveDirection::Left)
		{
			OEMaths::vec3f cameraRight = OEMaths::normalise(OEMaths::vec3f::cross(frontVec, cameraUp));
			currentPos -= cameraRight * velocity;
		}
		if (dir == MoveDirection::Right)
		{
			OEMaths::vec3f cameraRight = OEMaths::normalise(OEMaths::vec3f::cross(frontVec, cameraUp));
			currentPos += cameraRight * velocity;
		}

		updateViewMatrix();
	}
}

void Camera::updateViewMatrix()
{
	OEMaths::vec3f target = currentPos + frontVec;
	currentView = OEMaths::lookAt(currentPos, target, cameraUp);
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
	return currentPos;
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
	currentPos = pos;
}

}    // namespace OmegaEngine