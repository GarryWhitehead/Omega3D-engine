/*
**	A non-templeted maths library for speedier compile times.
**	(Who needs double vectors anyway?!)
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

// this should be defined if using the maths library with Vulkan to compensate for the difference in the y coord
#define USE_VULKAN_COORDS 1

namespace OEMaths
{

// some popular maths conversions (haven't decided were to locate these yet!)
template <typename T>
T radians(const T deg)
{
	return deg * static_cast<T>(M_PI / 180.0);
}

}    // namespace OEMaths
