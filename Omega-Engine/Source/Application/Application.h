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

#include "omega-engine/Application.h"

#include "Platforms/PlatformGlfw.h"

#include <cstdint>

// forward declerations
namespace OmegaEngine
{

class OEEngine;
class OEScene;
class OERenderer;
class OEWindowInstance;

class OEApplication : public Application
{
    
public:
    
    OEApplication() = default;

    OEApplication(const OEApplication&) = delete;
    OEApplication& operator=(const OEApplication&) = delete;

    static OEApplication* create(const char* title, uint32_t width, uint32_t height);

    static void destroy(OEApplication* app);

    OEEngine* createEngine(OEWindowInstance* window);

    /** 
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param: Title to use for the window. Nullptr states no title bar
     * @param width: window width in dpi; if zero will sets window width to fullscreen size 
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns a native window pointer
    */
	OEWindowInstance* init(const char* title, uint32_t width, uint32_t height);

    bool run(OEScene* scene, OERenderer* renderer);

    OEWindowInstance* getWindow();

private:

    // A engine instance. Only one permitted at the moment.
	OEEngine* engine = nullptr;

    // current window - maybe we should allow multiple window instances?
	OEWindowInstance* winInstance = nullptr;

    GlfwPlatform glfw;

    // the running state of this app. Set to true by 'esc' keypress or window close
    bool closeApp = false;
};

}
