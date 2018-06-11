#include "Engine/engine.h"
#include "utility/file_log.h"
#include "utility/message_handler.h"
#include "VulkanCore/VulkanEngine.h"
#include "Systems/input_system.h"
#include "Systems/camera_system.h"
#include "Systems/GraphicsSystem.h"
#include "Engine/world.h"
#include "glm.hpp"
#include "glfw/glfw3.h"

Engine::Engine() : 
	m_running(true),
	p_vkEngine(nullptr)
{
	CreateWindow("unnamed window");
}

Engine::Engine(const char *win_title) :	
	m_windowTitle(win_title),
	m_running(true),
	p_message(nullptr),
	p_vkEngine(nullptr)
{

CreateWindow(win_title);
}

Engine::~Engine()
{
	Release();
}

void Engine::CreateWindow(const char *winTitle)
{
	//glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		g_filelog->WriteLog("Critical error! Failed initialising GLFW. \n");
		exit(EXIT_FAILURE);
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_monitor = glfwGetPrimaryMonitor();
	m_vmode = glfwGetVideoMode(m_monitor);

	m_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, winTitle, m_monitor, nullptr);
	if (!m_window)
	{
		g_filelog->WriteLog("Critical error! Unable to open window!");
		exit(EXIT_FAILURE);
	}
}

void Engine::Init()
{
	// create message handling system
	p_message = new MessageHandler();

	// start by initialisng core vulkan components and creating a window
	p_vkEngine = new VulkanEngine(m_window, p_message);

	// Init the core vulkan framework
	p_vkEngine->InitVulkanCore();
}

void Engine::Update(int acc_time)
{
	//update component managers e.g. physics, transform, etc.
	m_worlds[m_currentWorldIndex]->UpdateComponentManagers();

	// update main world systems - e.g. camera, input, etc.
	m_worlds[m_currentWorldIndex]->UpdateSystems();

	p_vkEngine->Update(acc_time);
}

void Engine::Render(float interpolation)
{
	auto sys = GetSystem<GraphicsSystem>();
	sys->Render();
}

void Engine::Release()
{
	// destroy all generated worlds 
	for (auto& world : m_worlds) {
		world->Destroy();
	}

	// destroy vulkan framework
	if (p_vkEngine != nullptr) {
		delete p_vkEngine;
		p_vkEngine = nullptr;
	}

	// destroy mesage handler
	if (p_message != nullptr) {
		delete p_message;
		p_message = nullptr;
	}
	glfwTerminate();
}

void Engine::CreateWorld(std::vector<SystemId> systemIds, std::string name)
{
	assert(p_message != nullptr);
	assert(p_vkEngine != nullptr);

	World *world = new World(name, p_message);

	// generate all data assocaited with this world through serialising from file
	world->Generate(p_vkEngine);

	// register the main engine systems required - e.g. camera, input, collision
	world->RegisterSystems(systemIds, this, p_vkEngine);

	m_worlds.push_back(world);
	m_currentWorldIndex = m_worlds.size() - 1;
}






