/*
**	A non-templeted maths library for speedier compile times.
**	(Who needs double vectors anyway?!)
*/

#pragma once

#include "OEMaths/OEMaths_Vec2.h"
#include "OEMaths/OEMaths_Vec3.h"
#include "OEMaths/OEMaths_Vec4.h"
#include "OEMaths/OEMaths_Mat2.h"
#include "OEMaths/OEMaths_Mat3.h"
#include "OEMaths/OEMaths_Mat4.h"
#include "OEMaths/OEMaths_Quat.h"

#include <assert.h>
#include <cstdint>

#define M_PI 3.14159265358979323846264338327950288419716939937510582097
#define EPSILON 0.00001


namespace OEMaths
{

	// some popular maths conversions (haven't decided were to locate these yet!)
	float radians(const float deg);

}	
	
