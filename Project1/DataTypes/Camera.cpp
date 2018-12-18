#include "Camera.h"

#include "Omega_Global.h"
#include "Managers/EventManager.h"
#include <gtc/matrix_transform.hpp>
#include "GLFW/glfw3.h"

#include <algorithm>

using namespace OmegaEngine;

namespace OmegaEngine
{

	Camera::Camera(CameraDataType camera, uint32_t width, uint32_t height) :
		yaw(0.0f),
		pitch(0.0f),
		currentX(0.0),
		currentY(0.0),

	{
		zNear = camera.zNear;
		zFar = camera.zFar;
		fov = camera.fov;
		velocity = camera.velocity;
		upVec = camera.cameraUp;
		position = camera.position;
		type = camera.type;

		setPerspective(camera.fov, static_cast<float>(width / height), camera.zNear, camera.zFar);

		// set up events
		Global::managers.eventManager->registerListener<Camera, MouseButtonEvent>(this, mouse_button_event);
		Global::managers.eventManager->registerListener<Camera, KeyboardPressEvent>(this, keyboard_press_event);
	}

	void Camera::setNearFarPlane(float near, float far)
	{
		zNear = near;
		zFar = far;
	}

	void Camera::setPerspective(const float fov, const float aspect, const float zNear, const float zFar)
	{
		currentProjMatrix = glm::perspective(fov, aspect, zNear, zFar);
	}

	void Camera::updateViewMatrix()
	{
		currentViewMatrix = glm::lookAt(position, position + frontVec, upVec);
	}

	void Camera::mouse_button_event(MouseButtonEvent& event)
	{
		double offsetX = currentX - event.xpos;
		double offsetY = currentY - event.ypos;

		currentX = event.xpos;
		currentY = event.ypos;

		yaw -= offsetX * MOUSE_SENSITIVITY;
		pitch -= offsetY * MOUSE_SENSITIVITY;

		pitch = std::max(pitch, 89.0f);
		pitch = std::min(pitch, -89.0f);

		isMoving = true;
	}

	void Camera::keyboard_press_event(KeyboardPressEvent& event)
	{
		
		if (event.isMoving) {

			//calculate the pitch and yaw vectors
			frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			frontVec.y = sin(glm::radians(pitch));
			frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			frontVec = glm::normalize(frontVec);

			// check for forwards and strafe movement
			if (event.isMovingForward) {
				position += frontVec * velocity * Engine::DT;
			}
			if (event.isMovingBackward) {
				position -= frontVec * velocity * Engine::DT;
			}
			if (event.isMovingLeft) {
				position -= glm::normalize(glm::cross(frontVec, upVec)) * velocity * Engine::DT;
			}
			if (event.isMovingRight) {
				position += glm::normalize(glm::cross(frontVec, upVec)) * velocity * Engine::DT;
			}
				
			updateViewMatrix();

			isMoving = false;
		}

	}

}

