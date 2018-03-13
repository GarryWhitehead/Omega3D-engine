#ifndef _ENGINE_H
#define _ENGINE_H
#include <vector>
#include <memory>
#include <unordered_map>
#include <assert.h>

// forward declerations
class CameraSystem;
class InputSystem;
class VulkanScene;
struct GLFWwindow;

class Engine
{
public:

	const uint32_t SCREEN_WIDTH = 1280;
	const uint32_t SCREEN_HEIGHT = 768;
	const float CAMERA_FOV = 45.0f;
	const float DT = 1.0f / 30.0f;

	Engine();
	Engine(const char *winTitle);
	~Engine();

	// main core functions
	void Init();
	void Update();
	void Release();
	void Render(float interpolation);
	void InitSystems();

	// helper functions
	GLFWwindow* Window() const { return m_window; }

private:
	
	GLFWwindow* m_window;
	const char *m_windowTitle;

	bool m_running;
	
	VulkanScene *p_vulkanScene;

	// core system pointers
	std::shared_ptr<CameraSystem> p_cameraSystem;
	std::unique_ptr<InputSystem> p_inputSystem;
};

#endif // _ENGINE_H

