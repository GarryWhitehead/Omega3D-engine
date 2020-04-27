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
