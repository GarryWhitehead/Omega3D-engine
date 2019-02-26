#include "InputManager.h"
#include "Engine/Omega_Global.h"
#include "Managers/CameraManager.h"
#include "Managers/EventManager.h"
#include "GLFW/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <assert.h>

namespace OmegaEngine
{

	InputManager::InputManager(GLFWwindow *window, uint32_t width, uint32_t height) :
		left_button_press(false),
		right_button_press(false),
		cursorState(true)
	{
		assert(window != nullptr);
		// make glfw use our class for function calls
		glfwSetWindowUserPointer(window, this);

		// setup key response
		glfwSetKeyCallback(window, keyCallback);

		// centre cursor and disablefefre
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		//glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
		glfwSetCursorPos(window, width / 2, height / 2);

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

		if (key == GLFW_KEY_W) {							// forwrards (z-axis)
			event.isMovingForward = true;
		}
		if (key == GLFW_KEY_S) {							// backwards (z-axis)
			event.isMovingBackward = true;
		}
		if (key == GLFW_KEY_A) {							// leftwards movement (x-axis)
			event.isMovingLeft = true;
		}
		if (key == GLFW_KEY_D) {							// rightwards movement (x-axis)
			event.isMovingRight = true;
		}
		
		if (key == GLFW_KEY_ESCAPE) {												// exit

			glfwSetWindowShouldClose(window, true);
		}

		if (Global::eventManager()) {

			// update the camera position instantly so we reduce the chnace of lag
			Global::eventManager()->instantNotification<KeyboardPressEvent>(event);
		}
	}

	void InputManager::mouseButtonResponse(GLFWwindow *window, int button, int action, int mods)
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

	void InputManager::mouseMoveResponse(double xpos, double ypos)
	{
		MouseMoveEvent event{ xpos, ypos };

		if (Global::eventManager()) {

			if (left_button_press) {
				// update the camera position instantly so we reduce the chnace of lag
				Global::eventManager()->instantNotification<MouseMoveEvent>(event);
			}
		}
	}

	bool InputManager::buttonState(int button)
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
		InputManager *inputSys = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
		inputSys->keyResponse(window, key, scan_code, action, mode);
	}

	void mouseButtonPressCallback(GLFWwindow *window, int button, int action, int mods)
	{
		InputManager *inputSys = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
		inputSys->mouseButtonResponse(window, button, action, mods);
	}

	void mouseCallback(GLFWwindow *window, double xpos, double ypos)
	{
		InputManager *inputSys = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
		inputSys->mouseMoveResponse(xpos, ypos);
	}

}