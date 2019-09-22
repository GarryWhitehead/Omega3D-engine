#pragma once

#include "OEMaths/OEMaths.h"

#include <array>
#include <memory>
#include <tuple>
#include <unordered_map>

namespace OmegaEngine
{

// forward declerations
class World;
class ModelMaterial;
class ResourceManager;

class MaterialInfo
{

public:

	// this must be in the same order as the model material texture enum -
	// includes the binding number as per the model shader
	const std::vector<std::tuple<std::string, uint32_t>> textureExtensions = {
		{ "BaseColour", 0 },
		{ "Emissive", 3 },
		{ "MetallicRoughness", 2 },
		{ "Normal", 1 },
		{ "Occlusion", 4 }
	};

	MaterialInfo();
	~MaterialInfo();

	void buildTexture(ModelMaterial::TextureInfo& tex, ResourceManager& manager);

	void build(ModelMaterial& mat, World& world);

private:
	
	// This must not be empty as used for identifying textues when passed to the vulkan api
	std::string name;

	// the defining characteristics of this material
	ModelMaterial* material;

	bool doubleSided = false;

};

} // namespace OmegaEngine
