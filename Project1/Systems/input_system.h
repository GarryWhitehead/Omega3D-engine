#pragma once
#include <memory>
#include "Systems/system.h"

// forward declerations
struct GLFWwindow;
class CameraSystem;
class World;

class InputSystem : public System
{

public:

	InputSystem(World *world, GLFWwindow *window, uint32_t width, uint32_t height);
	void Init(GLFWwindow *window, uint32_t width, uint32_t height);
	~InputSystem();

	void Update() override;
	void Destroy() override;
	void KeyResponse(int key, int scan_code, int action, int mode);
	void MouseResponse(const double xpos, const double ypos);
	

private:

	World *p_world;
};

void keyCallback(GLFWwindow *window, int key, int scan_code, int action, int mode);
void mouseCallback(GLFWwindow *window, double xpos, double ypos);
