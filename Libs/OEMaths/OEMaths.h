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

#include "OEMaths_Mat2.h"
#include "OEMaths_Mat3.h"
#include "OEMaths_Mat4.h"
#include "OEMaths_MatN.h"
#include "OEMaths_Quat.h"
#include "OEMaths_Vec2.h"
#include "OEMaths_Vec3.h"
#include "OEMaths_Vec4.h"
#include "OEMaths_VecN.h"

#include <assert.h>
#include <cstdint>

//#define M_PI 3.14159265358979323846264338327950288419716939937510582097

//#define M_1_PI 0.31830988618379067154    // 1 / M_PI

#define M_DBL_PI 6.28318530718

#define M_HALF_PI 1.57079632679

#define M_EPSILON 0.00001

// this should be defined if using the maths library with Vulkan to compensate for the difference in
// the y coord
#define USE_VULKAN_COORDS 1

namespace OEMaths
{

// some popular maths conversions (haven't decided were to locate these yet!)
template <typename T>
T radians(const T deg)
{
    return deg * static_cast<T>(M_PI / 180.0);
}

} // namespace OEMaths
