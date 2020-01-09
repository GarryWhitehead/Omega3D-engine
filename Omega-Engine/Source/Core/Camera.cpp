#include "Camera.h"

#include "OEMaths/OEMaths_transform.h"

#include <algorithm>

namespace OmegaEngine
{

void OECamera::update()
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

void OECamera::updateViewMatrix()
{
	OEMaths::vec3f target = position + frontVec;
	currentView = OEMaths::lookAt(position, target, cameraUp);
}

void OECamera::rotate(float dx, float dy)
{
}

void OECamera::translate(float dx, float dy, float dz)
{
}

void OECamera::updateDirection(const OECamera::MoveDirection moveDir)
{
	dir = moveDir;
}

// =================== getters ===========================

OEMaths::mat4f OECamera::getMvpMatrix()
{
	return currentProj * currentView * currentModel;
}

float OECamera::getZNear() const
{
	return zNear;
}

float OECamera::getZFar() const
{
	return zFar;
}

float OECamera::getFov() const
{
	return fov;
}

OEMaths::vec3f& OECamera::getPos()
{
	return position;
}

OEMaths::mat4f& OECamera::getProjMatrix()
{
	return currentProj;
}

OEMaths::mat4f& OECamera::getViewMatrix()
{
	return currentView;
}

OEMaths::mat4f& OECamera::getModelMatrix()
{
	return currentModel;
}

// ========================= setters =============================
void OECamera::setPerspective()
{
	currentProj = OEMaths::perspective(fov, aspect, zNear, zFar);
}

void OECamera::setFov(const float camFov)
{
	fov = camFov;
}

void OECamera::setZNear(const float zn)
{
	zNear = zn;
}

void OECamera::setZFar(const float zf)
{
	zFar = zf;
}

void OECamera::setAspect(const float asp)
{
	aspect = asp;
}

void OECamera::setVelocity(const float vel)
{
	velocity = vel;
}

void OECamera::setType(const CameraType camType)
{
	type = camType;
}

void OECamera::setPosition(const OEMaths::vec3f& pos)
{
	position = pos;
}

}    // namespace OmegaEngine
