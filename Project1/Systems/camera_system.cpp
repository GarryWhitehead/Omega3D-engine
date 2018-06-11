#include "camera_system.h"
#include "Engine/engine.h"
#include "Engine/World.h"
#include "Systems/input_system.h"
#include <algorithm>
#include <gtc/matrix_transform.hpp>
#include "GLFW/glfw3.h"
#include <iostream>

CameraSystem::CameraSystem(World *world, MessageHandler *msg, glm::vec3 cameraPos, glm::vec3 cameraFront) :
	System(msg),
	p_world(world),
	m_cameraPos(cameraPos),
	m_cameraFront(cameraFront),
	m_currentDir(MoveDirection::NO_MOVEMENT),
	m_cameraUp(glm::vec3(0.0f, 1.0f, 0.0f)),
	m_cameraYaw(0.0f),
	m_cameraPitch(0.0f),
	m_currentX(0.0),
	m_currentY(0.0),
	m_isMoving(false),
	m_zNear(0.0f),
	m_zFar(0.0f)
{
	SetPerspective(Engine::CAMERA_FOV, static_cast<float>(Engine::SCREEN_WIDTH / Engine::SCREEN_HEIGHT), 0.1f, 512.0f);
	p_message->AddListener(ListenerID::CAMERA_MSG, NotifyResponse());
}

CameraSystem::~CameraSystem()
{
	Destroy();
}

void CameraSystem::SetMovementDirection(MoveDirection dir)
{
	m_currentDir = dir;
	m_isMoving = true;
}

void CameraSystem::SetPitchYaw(double xpos, double ypos)
{
	double offsetX = m_currentX - xpos;
	double offsetY = m_currentY - ypos;

	m_currentX = xpos;
	m_currentY = ypos;

	m_cameraYaw -= offsetX * MOUSE_SENSITIVITY;
	m_cameraPitch -= offsetY * MOUSE_SENSITIVITY;

	if (m_cameraPitch > 89.0f) {
		m_cameraPitch = 89.0f;
	}
	if (m_cameraPitch < -89.0f) {
		m_cameraPitch = -89.0f;
	}

	m_isMoving = true;
}

void CameraSystem::SetPerspective(const float fov, const float aspect, const float zNear, const float zFar)
{
	m_zNear = zNear;
	m_zFar = zFar;

	m_cameraInfo.projection = glm::perspective(fov, aspect, zNear, zFar);
}

void CameraSystem::UpdateViewMatrix()
{
	m_cameraInfo.viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
}

void CameraSystem::Update()
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
				m_cameraPos -= m_cameraFront *CAMERA_VELOCITY * Engine::DT;
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

void CameraSystem::Destroy()
{

}

void CameraSystem::OnNotify(Message& msg)
{

}
