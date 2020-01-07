#pragma once

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

struct AABBox
{
	/// 3D extents (min, max) of this box
	OEMaths::vec3f extents[2];
    
    static AABBox calculateRigidTransform(AABBox& box, OEMaths::mat4f world)
    {
        AABBox ret;
        OEMaths::mat3f rot = world.getRotation();
        OEMaths::vec3f trans = world.getTrans();
        ret.extents[0] = rot * box.extents[0] + trans;
        ret.extents[1] = rot * box.extents[1] + trans;
        return ret;
    }
    
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
