#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <assert.h>

enum class SystemId
{
	INPUT_SYSTEM_ID,
	CAMERA_SYSTEM_ID,
	GRAPHICS_SYSTEM_ID
};

// forward declerations
class CameraSystem;
class InputSystem;
class VulkanEngine;
struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;
class World;

class Engine
{
public:

	static const uint32_t SCREEN_WIDTH = 1280;
	static const uint32_t SCREEN_HEIGHT = 768;
	static constexpr float CAMERA_FOV = 45.0f;
	static constexpr float DT = 1.0f / 30.0f;

	Engine();
	Engine(const char *winTitle);
	~Engine();

	// main core functions
	void Init();
	void Update();
	void Release();
	void Render(float interpolation);
	void CreateWorld(std::vector<SystemId> systemIds, std::string name);
	void CreateWindow(const char *winTitle);

	template <typename T>
	T* GetSystem(SystemId id);

	// helper functions
	GLFWwindow* Window() const { return m_window; }

private:
	
	GLFWwindow* m_window;
	const char *m_windowTitle;
	GLFWmonitor* m_monitor;
	const GLFWvidmode* m_vmode;

	bool m_running;
	
	VulkanEngine *p_vkEngine;

	// a collection of worlds registered with the engine
	std::vector<World*> m_worlds;
	uint32_t m_currentWorldIndex;		// current world which will be rendered, updated, etc.
};

template <typename T>
T* Engine::GetSystem(SystemId id)
{
	T* sys = static_cast<T*>(m_worlds[m_currentWorldIndex]->HasSystem(id));
	assert(sys != nullptr);
	return sys;
}

