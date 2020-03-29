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
