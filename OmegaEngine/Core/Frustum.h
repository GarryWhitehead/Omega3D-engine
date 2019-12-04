#pragma once

#include "OEMaths/OEMaths.h"

#include <array>

namespace OmegaEngine
{

class Frustum
{
public:

	Frustum()
	{
	}

	void projection(OEMaths::mat4f viewProj);

	bool checkSphereIntersect(OEMaths::vec3f& pos, float radius);

	bool checkBoxIntersect(OEMaths::vec4f& box);

private:

	std::array<OEMaths::vec4f, 6> planes;
};

}    // namespace OmegaEngine