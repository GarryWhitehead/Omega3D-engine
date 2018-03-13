#include "engine.h"
#include "utility/file_log.h"
#include "VulkanCore/vulkan_scene.h"
#include "Systems/input_system.h"
#include "Systems/camera_system.h"
#include "glm.hpp"

Engine::Engine()
{
}

Engine::Engine(const char *win_title) :	
	m_windowTitle(win_title),
	m_running(true)
{
}

Engine::~Engine()
{
}

void Engine::Init()
{
	// start by initialisng core vulkan components and creating a window
	p_vulkanScene = new VulkanScene();
	
	// create the main window
	m_window = p_vulkanScene->InitWindow(m_windowTitle, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Init the core vulkan framework
	p_vulkanScene->InitVulkanCore();

	// initialise all the core systems
	this->InitSystems();

	// init view 
	p_cameraSystem->SetPerspective(CAMERA_FOV, static_cast<float>(SCREEN_WIDTH / SCREEN_HEIGHT), 0.1f, 512.0f);
	
	// initialise all the vulkan components required to draw the scene
	p_vulkanScene->Init();
}

void Engine::Update()
{
	p_inputSystem->Update();
	p_cameraSystem->Update(DT);
	p_vulkanScene->Update(p_cameraSystem.get());
}

void Engine::Render(float interpolation)
{
	p_vulkanScene->RenderFrame();
}

void Engine::Release()
{
	// goodbye vulkan main components - all other components will be dealt with by the deconstructors in the vulkan system
	delete p_vulkanScene;

	// and terminate the window
	glfwTerminate();
}

void Engine::InitSystems()
{
	p_cameraSystem = std::make_shared<CameraSystem>(glm::vec3(0.0f, 0.0f, -200.0f), DT);
	p_inputSystem = std::make_unique<InputSystem>(m_window, p_cameraSystem, SCREEN_WIDTH, SCREEN_HEIGHT);
}





