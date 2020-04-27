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

#include "Surface.h"

#ifdef VK_USE_PLATFORM_MACOS_MVK

#import <Foundation/Foundation.h>

#include "Cocoa/Cocoa.h"
#include "Metal/Metal.h"

#include <QuartzCore/QuartzCore.h>
#include "QuartzCore/CAMetalLayer.h"

#include <cassert>

namespace VulkanAPI
{
    
namespace Platform
{

SurfaceWrapper::SurfaceWrapper(OmegaEngine::OEWindowInstance& win, vk::Instance& instance)
{
    // we expect a NSWindow class, whcih will be converted to a NSView and subsequently a CAMetallayer
    NSWindow* nsWin  = (NSWindow*)win.getNativeWindowPtr();
    assert([nsWin isKindOfClass:[NSWindow class]]);
    
    NSView* view = [nsWin contentView];
    [view setWantsLayer:YES];
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.bounds = view.bounds;
    
    // these are required to ensure fullscreen mode is correctly displayed
    metalLayer.drawableSize = [view convertSizeToBacking:view.bounds.size];
    metalLayer.opaque = YES;
    
    [view setLayer:metalLayer];
    
    vk::MacOSSurfaceCreateInfoMVK createInfo { {}, (__bridge void*)view };
    VK_CHECK_RESULT(instance.createMacOSSurfaceMVK(&createInfo, nullptr, &surface));
}

}    // namespace Platform
}    // namespace VulkanAPI

#endif
