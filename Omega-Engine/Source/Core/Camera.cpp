#include "Camera.h"

#include "OEMaths/OEMaths_transform.h"

#include <algorithm>

namespace OmegaEngine
{
void OECamera::prepare()
{
    // set the projection matrix (perspective only for now)
    currentProj = OEMaths::perspective(fov, aspect, zNear, zFar);
}

void OECamera::update()
{
    if (dir != MoveDirection::None)
    {
        // calculate the pitch and yaw vectors
        frontVec = OEMaths::vec3f {
            std::cos(OEMaths::radians(rotation.x)) * std::cos(OEMaths::radians(rotation.y)),
            std::sin(OEMaths::radians(rotation.x)),
            std::cos(OEMaths::radians(rotation.x)) * std::sin(OEMaths::radians(rotation.y))};
        frontVec = OEMaths::normalise(frontVec);

        if (dir == MoveDirection::Up)
        {
            position += frontVec * velocity; // Engine::DT;	<- Needed?
        }
        if (dir == MoveDirection::Down)
        {
            position -= frontVec * velocity; // Engine::DT;
        }
        if (dir == MoveDirection::Left)
        {
            OEMaths::vec3f cameraRight =
                OEMaths::normalise(OEMaths::vec3f::cross(frontVec, cameraUp));
            position -= cameraRight * velocity;
        }
        if (dir == MoveDirection::Right)
        {
            OEMaths::vec3f cameraRight =
                OEMaths::normalise(OEMaths::vec3f::cross(frontVec, cameraUp));
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

void OECamera::setType(const Camera::CameraType camType)
{
    type = camType;
}

void OECamera::setPosition(const OEMaths::vec3f& pos)
{
    position = pos;
}

// ============================ front-end =====================================

void Camera::setFov(const float camFov)
{
    static_cast<OECamera*>(this)->setFov(camFov);
}

void Camera::setZNear(const float zn)
{
    static_cast<OECamera*>(this)->setZNear(zn);
}

void Camera::setZFar(const float zf)
{
    static_cast<OECamera*>(this)->setZFar(zf);
}

void Camera::setAspect(const float asp)
{
    static_cast<OECamera*>(this)->setAspect(asp);
}

void Camera::setVelocity(const float vel)
{
    static_cast<OECamera*>(this)->setVelocity(vel);
}

void Camera::setType(const CameraType camType)
{
    static_cast<OECamera*>(this)->setType(camType);
}

void Camera::setPosition(const OEMaths::vec3f& pos)
{
    static_cast<OECamera*>(this)->setPosition(pos);
}

OEMaths::mat4f Camera::getMvpMatrix()
{
    return static_cast<OECamera*>(this)->getMvpMatrix();
}

float Camera::getZNear() const
{
    return static_cast<const OECamera*>(this)->getZNear();
}

float Camera::getZFar() const
{
    return static_cast<const OECamera*>(this)->getZFar();
}

float Camera::getFov() const
{
    return static_cast<const OECamera*>(this)->getFov();
}

OEMaths::vec3f& Camera::getPos()
{
    return static_cast<OECamera*>(this)->getPos();
}

OEMaths::mat4f& Camera::getProjMatrix()
{
    return static_cast<OECamera*>(this)->getProjMatrix();
}

OEMaths::mat4f& Camera::getViewMatrix()
{
    return static_cast<OECamera*>(this)->getViewMatrix();
}

OEMaths::mat4f& Camera::getModelMatrix()
{
    return static_cast<OECamera*>(this)->getModelMatrix();
}

void Camera::prepare()
{
    static_cast<OECamera*>(this)->prepare();
}

} // namespace OmegaEngine
