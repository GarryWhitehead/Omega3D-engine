#pragma once

#include "OEMaths/OEMaths.h"

#include <array>

namespace OmegaEngine
{

//forward declerations
struct AABox;

class Frustum
{
public:

	Frustum()
	{
	}

	void projection(OEMaths::mat4f viewProj);

	bool checkSphereIntersect(OEMaths::vec3f& pos, float radius);

	bool checkBoxPlaneIntersect(AABox& box);

private:

	std::array<OEMaths::vec4f, 6> planes;
};

}    // namespace OmegaEngine