/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include "OEMaths/OEMaths.h"

#include "GLFW/glfw3.h"

// forward declerations
namespace OmegaEngine
{
class OEScene;
class OECamera;


class GlfwPlatform
{
public:

	GlfwPlatform();
	~GlfwPlatform();

	/**
		* @brief Initilaises glfw. Must be called first.
		* @return If successful, returns true.
		*/
	bool init();

	/**
		* @brief Creates a window object
		* @param width The desired width of the window in pixels. If this and **height** are zero, a borderless fullscreen window will be created. The monitor width will be output.
		* @param height The desired height of the window in pixels. If this and **width** are zero, a borderless fullscreen window will be created. The monitor height will be output.
		* @param title The title of the window. If nullptr, no title bar will be created
		* @return Whether the winodw was successfully created
		*/
	bool createWindow(uint32_t& width, uint32_t& height, const char* title);

	/**
		* @brief Checks for events in the queue.
		*/
	void poll();

	/**
		* @brief Gets all possible vulkan extensions for creating a vulkan surface object.
		* @return A tuple containing all surface extensions and the instance count. Null if an error occured.
		*/
	std::pair<const char**, uint32_t> getInstanceExt();

	/**
		* @brief Returns a native window pointer
		* @return A window pointer as void - i.e HWND for windows, XCB for linux
		*/
	void* getNativeWinPointer();

	// =================== window input ==========================================
	void keyResponse(GLFWwindow* window, int key, int scan_code, int action, int mode);
	void mouseButtonResponse(GLFWwindow* window, int button, int action, int mods);
	void mouseMoveResponse(double xpos, double ypos);

	bool buttonState(int button);
	void getCursorPos(GLFWwindow* window, double* xpos, double* ypos);

	void setCamera(OmegaEngine::OEScene& scene);

private:
	GLFWwindow* window = nullptr;
	GLFWmonitor* monitor = nullptr;
	const GLFWvidmode* vmode;

	// states of mouse actions
	bool leftMousePress = false;
	bool rightMousePress = false;

	// current mouse position
	OEMaths::vec2f mousePos = { 0.0f };

	// a pointer to the current camera held by the scene
	OmegaEngine::OECamera* camera = nullptr;
};

void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode);
void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);

}    // namespace OmegaEngine
