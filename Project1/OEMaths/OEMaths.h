#pragma once

#include <assert.h>

// include all the popular vertices and matrices 
#include "OEMaths_mat2.h"
#include "OEMaths_mat3.h"
#include "OEMaths_mat4.h"
#include "OEMaths_vec2.h"
#include "OEMaths_vec3.h"
#include "OEMaths_vec4.h"

#define M_PI 3.141592653589793238462643383279502884197169399375

namespace OEMaths
{
	
	// some popular maths conversions (haven't decided were to locate these yet!)
	template <typename Typename>
	Typename radians(const Typename deg)
	{
		return deg * M_PI / 180;
	}
}

