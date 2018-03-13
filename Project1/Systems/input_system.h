#pragma once
#include <memory>

// forward declerations
struct GLFWwindow;
class CameraSystem;

class InputSystem
{

public:

	InputSystem();
	InputSystem(GLFWwindow *window, std::shared_ptr<CameraSystem> camera, uint32_t width, uint32_t height);
	~InputSystem();

	void Update();
	void KeyResponse(int key, int scan_code, int action, int mode);
	void MouseResponse(double xpos, double ypos);
	

private:

	std::shared_ptr<CameraSystem> p_cameraSystem;
};

void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);
