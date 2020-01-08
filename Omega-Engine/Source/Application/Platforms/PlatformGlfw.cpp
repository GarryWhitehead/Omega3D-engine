#include "PlatformGlfw.h"

#include "Core/Camera.h"
#include "Core/Scene.h"

#include "GLFW/glfw3native.h"

GlfwPlatform::GlfwPlatform()
{
}

GlfwPlatform::~GlfwPlatform()
{
}

bool GlfwPlatform::init()
{
	if (!glfwInit())
	{
		return false;
	}
	return true;
}

bool GlfwPlatform::createWindow(uint32_t& width, uint32_t& height, const char* title)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// if no title specified, no window decorations will be used
	if (!title)
	{
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	}

	// if dimensions set to zero, get the primary monitor which will create a fullscreen, borderless window
	if (width == 0 && height == 0)
	{
		monitor = glfwGetPrimaryMonitor();
		vmode = glfwGetVideoMode(monitor);
		width = vmode->width;
		height = vmode->height;
	}

	window = glfwCreateWindow(width, height, title, monitor, nullptr);
	if (!window)
	{
		return false;
	}

	// ** set window inputs **
	glfwSetWindowUserPointer(window, this);
	glfwSetKeyCallback(window, keyCallback);                         // setup key response
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);     // centre cursor and disable cursor
	glfwSetCursorPosCallback(window, mouseCallback);                 // mouse movement feedback
	glfwSetMouseButtonCallback(window, mouseButtonPressCallback);    // mouse button callback

	return true;
}

void* GlfwPlatform::getNativeWinPointer()
{
#ifdef _WIN32
	return (void*)glfwGetWin32Window(window);
#endif
}

std::pair<const char**, uint32_t> GlfwPlatform::getInstanceExt()
{
	uint32_t count = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

	return std::make_pair(glfwExtensions, count);
}

// ========================== evevnt handling =================================================

void GlfwPlatform::poll()
{
	glfwPollEvents();
}

void GlfwPlatform::keyResponse(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
	switch (action)
	{
	case GLFW_PRESS:
	{
		if (key == GLFW_KEY_W)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::Up);    // forwrards (z-axis)
		}
		if (key == GLFW_KEY_S)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::Down);    // backwards (z-axis)
		}
		if (key == GLFW_KEY_A)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::Left);    // leftwards movement (x-axis)
		}
		if (key == GLFW_KEY_D)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::Right);    // rightwards movement (x-axis)
		}
		break;
	}
	case GLFW_RELEASE:
	{
		if (key == GLFW_KEY_W)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::None);
		}
		if (key == GLFW_KEY_S)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::None);
		}
		if (key == GLFW_KEY_A)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::None);
		}
		if (key == GLFW_KEY_D)
		{
			camera->updateDirection(OmegaEngine::Camera::MoveDirection::None);
		}
		break;
	}
	}

	// check for exit
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);    // exit
	}
}

void GlfwPlatform::mouseButtonResponse(GLFWwindow* window, int button, int action, int mods)
{
	// check the left mouse button
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			leftMousePress = true;
		}
		else
		{
			leftMousePress = false;
		}
	}

	// and whther the right has changed
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			rightMousePress = true;
		}
		else
		{
			rightMousePress = false;
		}
	}
}

void GlfwPlatform::mouseMoveResponse(double xpos, double ypos)
{
	float dx = mousePos.x - static_cast<float>(xpos);
	float dy = mousePos.y - static_cast<float>(ypos);

	mousePos = OEMaths::vec2f{ static_cast<float>(xpos),  static_cast<float>(ypos) };

	if (leftMousePress)
	{
		// update the camera position
		camera->rotate(dx, dy);
	}
	if (rightMousePress)
	{
		camera->translate(dx, dy, 0.0f);
	}
	// TODO: add zoom to the middle wheel
}

bool GlfwPlatform::buttonState(int button)
{
	bool state;

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		state = leftMousePress;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		state = rightMousePress;
	}

	return state;
}

void GlfwPlatform::getCursorPos(GLFWwindow* window, double* xpos, double* ypos)
{
	glfwGetCursorPos(window, xpos, ypos);
}

void GlfwPlatform::setCamera(OmegaEngine::Scene& scene)
{
	camera = scene.getCurrentCamera();
}

// ======================= static functions for glfw input. Kind of a hack to allow for accessing class member functions ==============================

void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
	GlfwPlatform* inputSys = reinterpret_cast<GlfwPlatform*>(glfwGetWindowUserPointer(window));
	inputSys->keyResponse(window, key, scan_code, action, mode);
}

void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods)
{
	GlfwPlatform* inputSys = reinterpret_cast<GlfwPlatform*>(glfwGetWindowUserPointer(window));
	inputSys->mouseButtonResponse(window, button, action, mods);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	GlfwPlatform* inputSys = reinterpret_cast<GlfwPlatform*>(glfwGetWindowUserPointer(window));
	inputSys->mouseMoveResponse(xpos, ypos);
}
