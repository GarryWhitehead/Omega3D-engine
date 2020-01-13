#include "Surface.h"

#ifdef VK_USE_PLATFORM_MACOS_MVK

#import <Foundation/Foundation.h>

#include "Cocoa/Cocoa.h"
#include "Metal/Metal.h"
#include "QuartzCore/CAMetalLayer.h"

#include <cassert>

namespace VulkanAPI
{
    
namespace Platform
{

SurfaceWrapper::SurfaceWrapper(OmegaEngine::OEWindowInstance& win, vk::Instance& instance)
{
    NSView* nsview = (__bridge NSView*) win.getNativeWindowPtr();
    assert(nsview);
    
    vk::MacOSSurfaceCreateInfoMVK createInfo { {}, (__bridge void*)nsview };
    instance.createMacOSSurfaceMVK(&createInfo, nullptr, &surface);
}

}    // namespace Platform
}    // namespace VulkanAPI

#endif
