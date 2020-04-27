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

#define VULKAN_HPP_TYPESAFE_CONVERSION
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

// sets whether we should use validation layers for debugging
#define VULKAN_VALIDATION_DEBUG 1

// threading info
#define VULKAN_THREADED 1
#define VULKAN_THREADED_GROUP_SIZE 512

#define VK_CHECK_RESULT(f)                                                                         \
    {                                                                                              \
        vk::Result res = (f);                                                                      \
        if (res != vk::Result::eSuccess)                                                           \
        {                                                                                          \
            printf(                                                                                \
                "Fatal : VkResult returned error code %i at %s at line %i.\n",                     \
                static_cast<int>(res),                                                             \
                __FILE__,                                                                          \
                __LINE__);                                                                         \
            assert(res == vk::Result::eSuccess);                                                   \
        }                                                                                          \
    }

// VMA doesn't use the c++ bindings, so this is specifically for dealing with VMA functions
#define VMA_CHECK_RESULT(f)                                                                        \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        if (res != VK_SUCCESS)                                                                     \
        {                                                                                          \
            printf(                                                                                \
                "Fatal : VkResult returned error code at %s at line %i.\n", __FILE__, __LINE__);   \
            assert(res == VK_SUCCESS);                                                             \
        }                                                                                          \
    }
