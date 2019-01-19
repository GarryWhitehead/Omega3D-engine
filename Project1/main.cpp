#include "utility/file_log.h"
#include "Engine/engine.h"
#include "windows.h"

#include "GLFW/glfw3.h"

#include "tiny_gltf.h"

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
	

	
	

	engine.Release();
}
