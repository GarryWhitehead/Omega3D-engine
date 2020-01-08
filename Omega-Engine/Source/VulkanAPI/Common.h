#pragma once

#define VULKAN_HPP_TYPESAFE_CONVERSION
#include "vulkan/vulkan.hpp"

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

// VMA doesn't use the c++ bindings, so this is specifically for dealing with VMA functions
#define VMA_CHECK_RESULT(f)                                                                          \
{                                                                                               \
    VkResult res = (f);                                                                       \
    if (res != VK_SUCCESS)                                                            \
    {                                                                                           \
        printf("Fatal : VkResult returned error code at %s at line %i.\n", __FILE__, __LINE__); \
        assert(res == VK_SUCCESS);                                                    \
    }                                                                                           \
}
