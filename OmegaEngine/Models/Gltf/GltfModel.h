#pragma once
#include "Models/ModelAnimation.h"
#include "Models/ModelImage.h"
#include "Models/ModelMaterial.h"
#include "Models/ModelMesh.h"
#include "Models/ModelNode.h"
#include "Models/ModelSkin.h"

#include "utility/String.h"

#include "OEMaths/OEMaths.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace OmegaEngine
{

// <extension name, value (as string)>
using ExtensionData = std::unordered_map<Util::String, Util::String>;

class GltfModel
{

public:
	// utility functions for dealing with gltf json data
	static bool prepareExtensions(const cgltf_extras& extras, cgltf_data& data, ExtensionData& extensions);
	static OEMaths::vec3f tokenToVec3(Util::String str);

	// atributes
	static void getAttributeData(const cgltf_attribute* attrib, uint8_t* base, size_t& stride);

	bool load(Util::String filename);

private:
	ModelNode nodes;
	ModelMaterial materials;
	ModelImage images;
	ModelSkin skins;
	ModelAnimation animations;

	ModelNode* getNode(uint32_t index)
	{
		ModelNode* foundNode = nullptr;
		for (auto& node : nodes)
		{
			foundNode = node->getNodeRecursive(index);
			if (foundNode)
			{
				break;
			}
		}
		return foundNode;
	}
};

}    // namespace OmegaEngine