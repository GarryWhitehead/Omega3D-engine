#pragma once
#include <memory>
#include "Systems/system.h"

// forward declerations
struct GLFWwindow;
class CameraSystem;
class World;
class MessageHandler;

class InputSystem : public System
{

public:

	InputSystem(World *world, MessageHandler *msg, GLFWwindow *window, uint32_t width, uint32_t height);
	virtual ~InputSystem();

	void Update() override;
	void Destroy() override;
	void OnNotify(Message& msg) override;

	void Init(GLFWwindow *window, uint32_t width, uint32_t height);
	void KeyResponse(int key, int scan_code, int action, int mode);
	void MouseButtonResponse(GLFWwindow *window, int button, int action, int mods);
	void MouseMoveResponse(double xpos, double ypos);
	
	// useful helper functions
	bool ButtonState(int button);
	void GetCursorPos(double *xpos, double *ypos);
	GLFWwindow* CurrentGLFWwindow() { return p_window; }
	void SwitchWindowCursorState();

private:

	World *p_world;
	GLFWwindow *p_window;

	bool left_button_press;
	bool right_button_press;
	bool cursorState;
};

void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode);
void mouseButtonPressCallback(GLFWwindow *window, int button, int action, int mods);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);
