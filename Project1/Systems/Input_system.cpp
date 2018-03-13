#include "input_system.h"
#include "Systems/camera_system.h"
#include "GLFW/glfw3.h"
#include <iostream>

InputSystem::InputSystem()
{
}

InputSystem::InputSystem(GLFWwindow *window, std::shared_ptr<CameraSystem> camera, uint32_t width, uint32_t height) : 
	p_cameraSystem(camera)
{
	// make glfw use our class for function calls
	glfwSetWindowUserPointer(window, this);

	// setup key response
	glfwSetKeyCallback(window, keyCallback);

	// centre cursor and disable
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, width / 2, height / 2);

	// and mouse movement feedback
	glfwSetCursorPosCallback(window, mouseCallback);
}

InputSystem::~InputSystem()
{
}

void InputSystem::Update()
{
	glfwPollEvents();
}

void InputSystem::KeyResponse(int key, int scan_code, int action, int mode)
{

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		
	}

	if (key == GLFW_KEY_W) {							// forwrards (z-axis)
		p_cameraSystem->SetMovementDirection(MoveDirection::MOVE_FORWARD);
	}
	if (key == GLFW_KEY_S) {							// backwards (z-axis)
		p_cameraSystem->SetMovementDirection(MoveDirection::MOVE_BACKWARD);
	}
	if (key == GLFW_KEY_A) {							// leftwards movement (x-axis)
		p_cameraSystem->SetMovementDirection(MoveDirection::MOVE_LEFT);
	}
	if (key == GLFW_KEY_D) {							// rightwards movement (x-axis)
		p_cameraSystem->SetMovementDirection(MoveDirection::MOVE_RIGHT);
	}
}

void InputSystem::MouseResponse(double xpos, double ypos)
{
	p_cameraSystem->SetPitchYaw(xpos, ypos);
}


// static functions for glfw input. Kind of a hack to allow for accessing class member function ===========================================================================================================================================================================================================

void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode)
{
	InputSystem *inputSys = reinterpret_cast<InputSystem*>(glfwGetWindowUserPointer(window));
	inputSys->KeyResponse(key, scan_code, action, mode);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
	InputSystem *inputSys = reinterpret_cast<InputSystem*>(glfwGetWindowUserPointer(window));
	inputSys->MouseResponse(xpos, ypos);
}