#pragma once

#include "omega-engine/Engine.h"

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
    
    void* getNativeWindowPtr();

    uint32_t getWidth() const;
    
    uint32_t getHeight() const;

    friend class OEEngine;
    friend class OEApplication;
    
private:
    
    void* nativeWin = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	std::pair<const char**, uint32_t> extensions;
    
};

}
