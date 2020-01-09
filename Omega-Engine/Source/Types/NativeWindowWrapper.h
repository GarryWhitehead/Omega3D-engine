#pragma once

#include "omega-engine/Engine.h"

#include "VulkanAPI/SwapChain.h"

#include <memory>
#include <cstdint>
#include <vector>

namespace OmegaEngine
{

/**
     * A wrapper containing all the information needed to create a swapchain.
     */
class OEWindowInstance : public WindowInstance
{
public:
    
    void* getNativeWindowPtr()
    {
        return nativeWin;
    }
    
    uint32_t getWidth() const
    {
        return width;
    }
    
    uint32_t getHeight() const
    {
        return height;
    }
    
    SwapchainHandle addSwapchain(VulkanAPI::Swapchain& sc)
    {
        swapchains.emplace_back(sc);
        return SwapchainHandle{static_cast<uint32_t>(swapchains.size() - 1)};
    }

    friend class OEEngine;
    friend class OEApplication;
    
private:
    
    void* nativeWin = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	std::pair<const char**, uint32_t> extensions;
    
    // keep a list of active swapchains here
    std::vector<VulkanAPI::Swapchain> swapchains;
};

}
