#include "input_system.h"
#include "Systems/camera_system.h"
#include "Engine/World.h"
#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <iostream>

InputSystem::InputSystem(World *world, MessageHandler *msg, GLFWwindow *window, uint32_t width, uint32_t height) :
	System(msg),
	p_world(world),
	left_button_press(false),
	right_button_press(false),
	cursorState(true)
{
	Init(window, width, height);
}


InputSystem::~InputSystem()
{
	Destroy();
}

void InputSystem::Init(GLFWwindow *window, uint32_t width, uint32_t height)
{	
	assert(window != nullptr);

	p_window = window;		// keep a shared pointer to the current GLFW window

	// make glfw use our class for function calls
	glfwSetWindowUserPointer(window, this);

	// setup key response
	glfwSetKeyCallback(window, keyCallback);

	// centre cursor and disable
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
	glfwSetCursorPos(window, width / 2, height / 2);

	// mouse movement feedback
	glfwSetCursorPosCallback(window, mouseCallback);

	// mouse button callback
	glfwSetMouseButtonCallback(window, mouseButtonPressCallback);

	// register with message handler
	p_message->AddListener(ListenerID::INPUT_MSG, NotifyResponse());
}

void InputSystem::Update()
{
	glfwPollEvents();
}

void InputSystem::Destroy()
{

}

void InputSystem::KeyResponse(int key, int scan_code, int action, int mode)
{
	auto p_cameraSystem = p_world->RequestSystem<CameraSystem>();

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
	if (key == GLFW_KEY_H && action == GLFW_PRESS) {							// enable/disable GUI
		
		p_message->AddMessage({ "SwitchGUI", ListenerID::VULKAN_MSG });
	}
	if (key == GLFW_KEY_ESCAPE) {												// exit

		glfwSetWindowShouldClose(p_window, true);
	}
}

void InputSystem::MouseButtonResponse(GLFWwindow *window, int button, int action, int mods)
{	
	// check the left mouse button
	if (button == GLFW_MOUSE_BUTTON_LEFT) {

		if (action == GLFW_PRESS) {

			left_button_press = true;

		}
		else {

			left_button_press = false;
		}
	}

	// and whther the right has changed
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {

		if (action == GLFW_PRESS) {

			right_button_press = true;

		}
		else {

			right_button_press = false;
		}
	}
}

void InputSystem::MouseMoveResponse(double xpos, double ypos)
{
	p_world->RequestSystem<CameraSystem>()->SetPitchYaw(xpos, ypos);
}

bool InputSystem::ButtonState(int button)
{
	bool state;

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		state = left_button_press;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		state = right_button_press;
	}

	return state;
}

void InputSystem::GetCursorPos(double *xpos, double *ypos)
{
	glfwGetCursorPos(p_window, xpos, ypos);	
}

void InputSystem::SwitchWindowCursorState()
{
	cursorState ^= 1;
	glfwSetInputMode(p_window, GLFW_CURSOR, cursorState ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void InputSystem::OnNotify(Message& msg)
{

}

// static functions for glfw input. Kind of a hack to allow for accessing class member functions ===========================================================================================================================================================================================================

void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode)
{
	InputSystem *inputSys = reinterpret_cast<InputSystem*>(glfwGetWindowUserPointer(window));
	inputSys->KeyResponse(key, scan_code, action, mode);
}

void mouseButtonPressCallback(GLFWwindow *window, int button, int action, int mods)
{
	InputSystem *inputSys = reinterpret_cast<InputSystem*>(glfwGetWindowUserPointer(window));
	inputSys->MouseButtonResponse(window, button, action, mods);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
	InputSystem *inputSys = reinterpret_cast<InputSystem*>(glfwGetWindowUserPointer(window));
	inputSys->MouseMoveResponse(xpos, ypos);
}