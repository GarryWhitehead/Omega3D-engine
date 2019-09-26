#pragma once
#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include <vector>
#include <unordered_map>

namespace OmegaEngine
{

class ModelSkin
{
public:

	ModelSkin() 
	{
	}

	~ModelSkin()
	{
	}
	 
	// copying and moving allowed
	ModelSkin(const ModelSkin&) = default;
	ModelSkin& operator=(const ModelSkin&) = default;
	ModelSkin(ModelSkin&&) = default;
	ModelSkin& operator=(ModelSkin&&) = default;

	bool prepare(cgltf_skin& skin);

private:

	Util::String name;
	
	// links the bone node name with the inverse transform
	std::unordered_map<Util::String, OEMaths::mat4f> invBindMatrices;
	
	// the node name indicating the root of the skeleton
	Util::String skeletonRoot;

	std::vector<OEMaths::mat4f> jointMatrices;
};

} // namespace OmegaEngine
