#include "Application.h"

#include "utility/Timer.h"

#include <thread>

Application::Application()
{
}

Application::~Application()
{
}

bool Application::init(const char* title, uint32_t width, uint32_t height, OmegaEngine::NativeWindowWrapper& output)
{
	// init glfw
	if (!glfw.init())
	{
		return nullptr;
	}

	// create a window
	if (!glfw.createWindow(width, height, title))
	{
		return nullptr;
	}

	output.width = width;
	output.height = height;
	output.nativeWin = (void*)glfw.getNativeWinPointer();
	output.extensions = glfw.getInstanceExt();
}

void Application::run()
{
	// convert delta time to ms
	const NanoSeconds frameTime(33ms);

	Util::Timer<NanoSeconds> timer;

	while (!closeApp)
	{
		NanoSeconds startTime = timer.getCurrentTime();

		// check for any input from the window
		glfw.poll();

		NanoSeconds endTime = timer.getCurrentTime();
		NanoSeconds elapsedTime = endTime - startTime;

		// if we haven't used up the frame time, sleep for remainder
		if (elapsedTime < frameTime)
		{
			std::this_thread::sleep_for(frameTime - elapsedTime);
		}
	}
}