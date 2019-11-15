#pragma once

#define VULKAN_HPP_TYPESAFE_CONVERSION
#include "vulkan/vulkan.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// sets whether we should use validation layers for debugging
#define VULKAN_VALIDATION_DEBUG 1

// threading info
#define VULKAN_THREADED 1
#define VULKAN_THREADED_GROUP_SIZE 512

#define VK_CHECK_RESULT(f)                                                                          \
	{                                                                                               \
		vk::Result res = (f);                                                                       \
		if (res != vk::Result::eSuccess)                                                            \
		{                                                                                           \
			printf("Fatal : VkResult returned error code at %s at line %i.\n", __FILE__, __LINE__); \
			assert(res == vk::Result::eSuccess);                                                    \
		}                                                                                           \
	}
