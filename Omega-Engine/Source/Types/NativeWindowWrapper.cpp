#include "NativeWindowWrapper.h"

#include "VulkanAPI/SwapChain.h"

namespace OmegaEngine
{

void* OEWindowInstance::getNativeWindowPtr()
{
	return nativeWin;
}

uint32_t OEWindowInstance::getWidth() const
{
	return width;
}

uint32_t OEWindowInstance::getHeight() const
{
	return height;
}

// ====================== front-end ==================================

void* WindowInstance::getNativeWindowPtr()
{
	return static_cast<OEWindowInstance*>(this)->getNativeWindowPtr();
}

uint32_t WindowInstance::getWidth() const
{
	return static_cast<const OEWindowInstance*>(this)->getWidth();
}

uint32_t WindowInstance::getHeight() const
{
	return static_cast<const OEWindowInstance*>(this)->getHeight();
}

}    // namespace OmegaEngine