#include "utility/file_log.h"
#include "Engine/engine.h"
#include "windows.h"
#include "GLFW/glfw3.h"
#include <memory>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
	const int fps = 60;
	const int frameLength = 1000 / fps;
	
	Engine engine("Omega3D v1.0");
	FileLog filelog("test-log", (int)FileLogFlags::FILELOG_CLOSE_AFTER_EACH_WRITE);
	
	// initiliases the window and vulkan main framework
	engine.Init();

	// the engine stores all the worlds which have there own designated systems but can share entities and data
	engine.CreateWorld({ SystemId::CAMERA_SYSTEM_ID, SystemId::INPUT_SYSTEM_ID, SystemId::GRAPHICS_SYSTEM_ID  }, "main_world");
	
	// fixed-step game loop
	float interpolation = 0;
	int accumulator = 0;
	long frameCount = 0, longestTime = 0, tickCount = 0, fpsElapsedTime = 0;
	auto endTime = std::chrono::steady_clock::now();
	
	while (!glfwWindowShouldClose(engine.Window()))
	{
		auto startTime = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(startTime - endTime);
		endTime = startTime;
		accumulator += static_cast<int>(elapsedTime.count());
		if (elapsedTime.count() == 0) {
			Sleep(5);			// TODO:: Get rid of this - not an ideal way to use up ticks!
		}

		while (accumulator >= frameLength) {
			engine.Update(accumulator);
			
			accumulator -= frameLength;
			tickCount++;
		}

		engine.Render(interpolation);
		frameCount++;

		fpsElapsedTime += static_cast<long>(elapsedTime.count());
		if (fpsElapsedTime >= 1000) {
			std::cout << "Frame count = " << frameCount << "\t\tTick count = " << tickCount << "\n";
			frameCount = 0; tickCount = 0; fpsElapsedTime = 0;
		}

	}

	engine.Release();
}
