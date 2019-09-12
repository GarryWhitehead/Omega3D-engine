#pragma once

#include <tuple>
#include <cstdint>

namespace OmegaEngine
{

/**
     * A wrapper containing all the information needed to create a swapchain.
     */
struct NativeWindowWrapper
{
	void* nativeWin;
	uint32_t width;
	uint32_t height;
	std::tuple<const char**, uint32_t> extensions;
};

}