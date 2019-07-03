#include "InputManager.h"
#include "Engine/Omega_Global.h"
#include "GLFW/glfw3.h"
#include "Managers/CameraManager.h"
#include "Managers/EventManager.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <assert.h>

namespace OmegaEngine
{

InputManager::InputManager(GLFWwindow *window, uint32_t width, uint32_t height)
    : leftButtonPress(false)
    , rightButtonPress(false)
    , cursorState(true)
{
	assert(window != nullptr);
	// make glfw use our class for function calls
	glfwSetWindowUserPointer(window, this);

	// setup key response
	glfwSetKeyCallback(window, keyCallback);

	// centre cursor and disablefefre
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//glfwSetCursorPos(window, width / 2, height / 2);

	// mouse movement feedback
	glfwSetCursorPosCallback(window, mouseCallback);

	// mouse button callback
	glfwSetMouseButtonCallback(window, mouseButtonPressCallback);
}

void InputManager::update()
{
	glfwPollEvents();
}

void InputManager::keyResponse(GLFWwindow *window, int key, int scan_code, int action, int mode)
{
	KeyboardPressEvent event;

	if (key == GLFW_KEY_W)
	{
		event.isMovingForward = true; // forwrards (z-axis)
	}
	if (key == GLFW_KEY_S)
	{
		event.isMovingBackward = true; // backwards (z-axis)
	}
	if (key == GLFW_KEY_A)
	{
		event.isMovingLeft = true; // leftwards movement (x-axis)
	}
	if (key == GLFW_KEY_D)
	{
		event.isMovingRight = true; // rightwards movement (x-axis)
	}

	if (key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(window, true); // exit
	}

	if (Global::eventManager())
	{
		// update the camera position instantly so we reduce the chnace of lag
		Global::eventManager()->instantNotification<KeyboardPressEvent>(event);
	}
}

void InputManager::mouseButtonResponse(GLFWwindow *window, int button, int action, int mods)
{
	// check the left mouse button
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			leftButtonPress = true;
		}
		else
		{
			leftButtonPress = false;
		}
	}

	// and whther the right has changed
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			rightButtonPress = true;
		}
		else
		{
			rightButtonPress = false;
		}
	}
}

void InputManager::mouseMoveResponse(double xpos, double ypos)
{
	MouseMoveEvent event{ xpos, ypos };

	if (Global::eventManager())
	{
		if (leftButtonPress)
		{
			// update the camera position instantly so we reduce the chnace of lag
			Global::eventManager()->instantNotification<MouseMoveEvent>(event);
		}
	}
}

bool InputManager::buttonState(int button)
{
	bool state;

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		state = leftButtonPress;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		state = rightButtonPress;
	}

	return state;
}

void InputManager::getCursorPos(GLFWwindow *window, double *xpos, double *ypos)
{
	glfwGetCursorPos(window, xpos, ypos);
}

void InputManager::switchWindowCursorState(GLFWwindow *window)
{
	cursorState ^= 1;
	glfwSetInputMode(window, GLFW_CURSOR, cursorState ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

// static functions for glfw input. Kind of a hack to allow for accessing class member functions ===========================================================================================================================================================================================================

void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode)
{
	InputManager *inputSys = reinterpret_cast<InputManager *>(glfwGetWindowUserPointer(window));
	inputSys->keyResponse(window, key, scan_code, action, mode);
}

void mouseButtonPressCallback(GLFWwindow *window, int button, int action, int mods)
{
	InputManager *inputSys = reinterpret_cast<InputManager *>(glfwGetWindowUserPointer(window));
	inputSys->mouseButtonResponse(window, button, action, mods);
}

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
	InputManager *inputSys = reinterpret_cast<InputManager *>(glfwGetWindowUserPointer(window));
	inputSys->mouseMoveResponse(xpos, ypos);
}

} // namespace OmegaEngine