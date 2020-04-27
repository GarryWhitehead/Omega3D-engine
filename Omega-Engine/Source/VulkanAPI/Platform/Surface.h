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

#include "VulkanAPI/Common.h"

#include "Types/NativeWindowWrapper.h"

namespace VulkanAPI
{

namespace Platform
{

/**
* @brief A base wrapper for platform specific vulkan surface objects
*/
class SurfaceWrapper
{
public:

    SurfaceWrapper() = default;
    SurfaceWrapper(OmegaEngine::OEWindowInstance& window, vk::Instance& instance);
    
	uint32_t getWidth() const
	{
		return winWidth;
	}

	uint32_t getHeight() const
	{
		return winHeight;
	}

	vk::SurfaceKHR& get()
	{
		return surface;
	}

public:
    
	vk::SurfaceKHR surface;

	uint32_t winWidth = 0;
	uint32_t winHeight = 0;
};

}    // namespace Platform
}    // namespace VulkanAPI
