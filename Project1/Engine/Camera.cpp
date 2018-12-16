#include "Camera.h"

#include <gtc/matrix_transform.hpp>
#include "GLFW/glfw3.h"

#include <algorithm>

namespace OmegaEngine
{

	Camera::Camera(CameraDataType camera, uint32_t width, uint32_t height) :
		direction(MoveDirection::NO_MOVEMENT),
		yaw(0.0f),
		pitch(0.0f),
		currentX(0.0),
		currentY(0.0),
		isMoving(false)

	{
		zNear = camera.zNear;
		zFar = camera.zFar;
		fov = camera.fov;
		velocity = camera.velocity;
		upVec = camera.cameraUp;
		position = camera.position;
		type = camera.type;

		setPerspective(camera.fov, static_cast<float>(width / height), camera.zNear, camera.zFar);
	}

	void Camera::setMovementDirection(MoveDirection dir)
	{
		direction = dir;
		isMoving = true;
	}

	void Camera::setPitchYaw(double xpos, double ypos)
	{
		double offsetX = currentX - xpos;
		double offsetY = currentY - ypos;

		currentX = xpos;
		currentY = ypos;

		yaw -= offsetX * MOUSE_SENSITIVITY;
		pitch -= offsetY * MOUSE_SENSITIVITY;

		pitch = std::max(pitch, 89.0f);
		pitch = std::min(pitch, -89.0f);

		isMoving = true;
	}

	void Camera::setPerspective(const float fov, const float aspect, const float zNear, const float zFar)
	{
		currentProjMatrix = glm::perspective(fov, aspect, zNear, zFar);
	}

	void Camera::updateViewMatrix()
	{
		currentViewMatrix = glm::lookAt(position, position + frontVec, upVec);
	}

	void Camera::update()
	{
		auto p_input = p_world->RequestSystem<InputSystem>();

		// check whther the left hand mouse button is pressed
		if (p_input->ButtonState(GLFW_MOUSE_BUTTON_LEFT)) {

			double xpos, ypos;
			p_input->GetCursorPos(&xpos, &ypos);

			SetPitchYaw(xpos, ypos);		// if pressed, adjust view according to changes in cursor pos
		}

		if (m_isMoving) {

			//calculate the pitch and yaw vectors
			m_cameraFront.x = cos(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
			m_cameraFront.y = sin(glm::radians(m_cameraPitch));
			m_cameraFront.z = sin(glm::radians(m_cameraYaw)) * cos(glm::radians(m_cameraPitch));
			m_cameraFront = glm::normalize(m_cameraFront);

			// check for forwards and strafe movement
			if (m_currentDir != MoveDirection::NO_MOVEMENT) {

				if (m_currentDir == MoveDirection::MOVE_FORWARD) {
					m_cameraPos += m_cameraFront * CAMERA_VELOCITY * Engine::DT;
				}
				if (m_currentDir == MoveDirection::MOVE_BACKWARD) {
					m_cameraPos -= m_cameraFront * CAMERA_VELOCITY * Engine::DT;
				}
				if (m_currentDir == MoveDirection::MOVE_LEFT) {
					m_cameraPos -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * CAMERA_VELOCITY * Engine::DT;
				}
				if (m_currentDir == MoveDirection::MOVE_RIGHT) {
					m_cameraPos += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * CAMERA_VELOCITY * Engine::DT;
				}
				m_currentDir = MoveDirection::NO_MOVEMENT;
			}

			this->UpdateViewMatrix();

			m_isMoving = false;
		}

	}

}

