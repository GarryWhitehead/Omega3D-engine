#include "CameraManager.h"
#include "Engine/Omega_Global.h"


namespace OmegaEngine
{

	CameraManager::CameraManager()
	{
		// set up events
		Global::managers.eventManager->registerListener<Camera, MouseButtonEvent>(this, mouse_button_event);
		Global::managers.eventManager->registerListener<Camera, KeyboardPressEvent>(this, keyboard_press_event);
	}


	CameraManager::~CameraManager()
	{
	}

	void CameraManager::updateViewMatrix()
	{
		currentViewMatrix = OEMaths::lookAt(position, position + frontVec, upVec);
	}

	void CameraManager::mouse_button_event(MouseButtonEvent& event)
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

	void CameraManager::keyboard_press_event(KeyboardPressEvent& event)
	{

		if (event.isMoving) {

			//calculate the pitch and yaw vectors
			frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			frontVec.y = sin(glm::radians(pitch));
			frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			frontVec = OEMaths::normalise(frontVec);

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