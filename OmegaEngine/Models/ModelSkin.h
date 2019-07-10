#pragma once
#include "OEMaths/OEMaths.h"

#include <string>
#include <vector>

namespace OmegaEngine
{

struct ModelSkin
{
	std::string name;
	std::vector<OEMaths::mat4f> invBindMatrices;
	std::vector<OEMaths::mat4f> jointMatrices;
};

} // namespace OmegaEngine
