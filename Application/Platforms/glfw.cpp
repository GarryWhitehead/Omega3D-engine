#include "glfw.h"

namespace Application
{
bool GlfwPlatform::init()
{
    if (!glfwInit())
    {
        return false;
    }
    return true;
}

bool GlfwPlatform::createWindow()
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    monitor = glfwGetPrimaryMonitor();
    vmode = glfwGetVideoMode(monitor);

    window = glfwCreateWindow(windowWidth, windowHeight, winTitle.c_str(), nullptr, nullptr);
    if (!window)
    {
        return false;
    }

    // ** set window inputs **
    glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, keyCallback);                        // setup key response
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);    // centre cursor and disable cursor
    glfwSetCursorPosCallback(window, mouseCallback);                // mouse movement feedback
    glfwSetMouseButtonCallback(window, mouseButtonPressCallback);   // mouse button callback

    return true;
}

bool GlfwPlatform::createSurfaceKHR(Device& device)
{
    // create a which will be the abstract scrren surface which will be used for creating swapchains
    // TODO: add more cross-platform compatibility by adding more surfaceCount
    VkSurfaceKHR tempSurface;
    VkResult err = glfwCreateWindowSurface(device->getInstance(), window, nullptr, &tempSurface);
    if (err)
    {
        return false;
    }
    device->setWindowSurface(vk::SurfaceKHR(tempSurface));
}

void GlfwPlatform::createVkInstance(Device& device)
{
    uint32_t instanceCount;
	const char **instanceExt = glfwGetRequiredInstanceExtensions(&instanceCount);
	device->createInstance(instanceExt, instanceCount);
}

void GlfwPlatform::poll()
{
	glfwPollEvents();
}

void GlfwPlatform::keyResponse(GLFWwindow *window, int key, int scan_code, int action, int mode)
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

void GlfwPlatform::mouseMoveResponse(double xpos, double ypos)
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

bool GlfwPlatform::buttonState(int button)
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

void GlfwPlatform::getCursorPos(GLFWwindow *window, double *xpos, double *ypos)
{
	glfwGetCursorPos(window, xpos, ypos);
}

void GlfwPlatform::switchWindowCursorState(GLFWwindow *window)
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

// ** helper functions **

void GlfwPlatform::setWindowWidth(const uint32_t width)
{
    winWidth = width;
}

void GlfwPlatform::setWindowHeight(const uint32_t height)
{
    winHeight = height;
}

}