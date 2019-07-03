#pragma once
#include <memory>

// forward declerations
struct GLFWwindow;

namespace OmegaEngine
{

class InputManager
{

public:
	InputManager(GLFWwindow *window, uint32_t width, uint32_t height);

	void update();
	void keyResponse(GLFWwindow *window, int key, int scan_code, int action, int mode);
	void mouseButtonResponse(GLFWwindow *window, int button, int action, int mods);
	void mouseMoveResponse(double xpos, double ypos);

	// useful helper functions
	bool buttonState(int button);
	void getCursorPos(GLFWwindow *window, double *xpos, double *ypos);
	void switchWindowCursorState(GLFWwindow *window);

private:
	bool leftButtonPress;
	bool rightButtonPress;
	bool cursorState;
};

// static functions for callbacks
void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode);
void mouseButtonPressCallback(GLFWwindow *window, int button, int action, int mods);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);

} // namespace OmegaEngine