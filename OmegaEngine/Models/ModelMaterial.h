#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

#include <cstdint>


namespace OmegaEngine
{

enum TextureType : size_t
{
	BaseColour,
	Emissive,
	MetallicRoughness,
	Normal,
	Occlusion,
	Count
};

class ModelMaterial
{
public:

	ModelMaterial();
	~ModelMaterial();

	// copyable and moveable
	ModelMaterial(const ModelMaterial&) = default;
	ModelMaterial& operator=(const ModelMaterial&) = default;
	ModelMaterial(ModelMaterial&&) = default;
	ModelMaterial& operator=(ModelMaterial&&) = default;

	Util::String getTextureUri(cgltf_texture_view& view);

	bool prepare(cgltf_material& mat);

	bool prepare(aiMaterial* mat);

	// helper functions
	Util::String getName()
	{
		assert(!name.empty());    // sanity check!
		return name;
	}

	friend class MaterialManager;

private:

	// used to identify this material. 
	Util::String name;

	// used to find the texture group in the list
	size_t bufferIndex;

	struct Factors
	{
		OEMaths::vec3f emissive = OEMaths::vec3f{ 1.0f };
		OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f };
		OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f };
		OEMaths::vec3f specular = OEMaths::vec3f{ 0.0f };

		float specularGlossiness = 1.0f;
		float roughness = 1.0f;
		float metallic = 1.0f;

	} factors;

	struct AlphaBlending
	{
		Util::String mask;
		float alphaMaskCutOff = 1.0f;

	} blending;

	// the paths for all textures. Empty paths signify that this texture isn't used
	Util::String texturePaths[TextureType::Count];

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;

	bool doubleSided = false;
};

}    // namespace OmegaEngine
