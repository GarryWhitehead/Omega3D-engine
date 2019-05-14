#include "OEMaths_convert.h"
#include "OEMaths.h"

namespace OEMaths
{
	// some popular maths conversions (haven't decided were to locate these yet!)
	float radians(const float deg)
	{
		return deg * (float)M_PI / 180.0f;
	}
}