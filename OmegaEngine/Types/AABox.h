#pragma once

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

struct AABBox
{
	/// 3D extents (min, max) of this box
	OEMaths::vec3f extents[2];

	/**
	* Calculates the center position of the box
	*/
	OEMaths::vec3f getCenter()
	{
		return (extents[1] + extents[0]) * OEMaths::vec3f{ 0.5f };
	}

	/**
	* Calculates the half extent of the box
	*/
	OEMaths::vec3f getHalfExtent()
	{
		return (extents[1] - extents[0]) * OEMaths::vec3f{ 0.5f };
	}

};

}    // namespace OmegaEngine