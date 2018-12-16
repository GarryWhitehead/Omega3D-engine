#include "Engine/engine.h"
#include "utility/file_log.h"
#include "utility/message_handler.h"
#include "Systems/InputManager.h"
#include "Systems/GraphicsSystem.h"
#include "Engine/world.h"
#include "Engine/Omega_Global.h"

#include "glm.hpp"
#include "glfw/glfw3.h"

namespace OmegaEngine
{
	Engine::Engine(const char *win_title, uint32_t width, uint32_t height) :
		windowTitle(win_title),
		windowWidth(width),
		windowHeight(height),
		isRunning(true),
	{
		// Create a new instance of glfw
		createWindow(win_title);

		// create all global instances including managers
		Global::init();
		
		//create a new instance of the input manager
		inputManager = std::make_unique<InputManager>(window, width, height);
	}

	Engine::~Engine()
	{
		release();
	}

	void Engine::createWindow(const char *winTitle)
	{
		//glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
		{
			g_filelog->WriteLog("Critical error! Failed initialising GLFW. \n");
			exit(EXIT_FAILURE);
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		monitor = glfwGetPrimaryMonitor();
		vmode = glfwGetVideoMode(monitor);

		window = glfwCreateWindow(windowWidth, windowHeight, winTitle, nullptr, nullptr);
		if (!window)
		{
			g_filelog->WriteLog("Critical error! Unable to open window!");
			exit(EXIT_FAILURE);
		}+
	}

	void Engine::update(int acc_time)
	{
		//update component managers e.g. physics, transform, etc.
		m_worlds[m_currentWorldIndex]->UpdateComponentManagers();

		// update main world systems - e.g. camera, input, etc.
		m_worlds[m_currentWorldIndex]->UpdateSystems();

		p_vkEngine->Update(acc_time);
	}

	void Engine::render(float interpolation)
	{
		auto sys = GetSystem<GraphicsSystem>();
		sys->Render();
	}

	void Engine::release()
	{
		// destroy all generated worlds 
		for (auto& world : m_worlds) {
			world->Destroy();
		}

		glfwTerminate();
	}

	void Engine::createWorld(std::string filename, std::string name)
	{
		std::unique_ptr<World> *world = std::make_unique<World>(filename, name);

		m_worlds.push_back(world);
		m_currentWorldIndex = m_worlds.size() - 1;
	}
}





