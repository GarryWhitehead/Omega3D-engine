#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

#include <cstdint>


namespace OmegaEngine
{

class ModelMaterial
{
public:
	enum class TextureId
	{
		BaseColour,
		Emissive,
		MetallicRoughness,
		Normal,
		Occlusion,
		Count
	};

	ModelMaterial();
	~ModelMaterial();

	Util::String getTextureUri(cgltf_texture_view& view);

	bool prepare(cgltf_material& mat, ExtensionData& extensions);

	bool prepare(aiMaterial* mat);

	// helper functions
	Util::String getName()
	{
		assert(!name.empty());    // sanity check!
		return name;
	}

	void updateIndex(size_t index)
	{
		bufferIndex = index;
	}

	friend class MaterialInfo;

private:
	// used to identify this material. Must be set!
	Util::String name;

	// index into container
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

	struct TexCoordSets
	{
		uint32_t baseColour = 0;
		uint32_t metallicRoughness = 0;
		uint32_t normal = 0;
		uint32_t emissive = 0;
		uint32_t occlusion = 0;
		uint32_t specularGlossiness = 0;
		uint32_t diffuse = 0;
	} uvSets;

	/**
	 * @brief Filenames which will be passed to the resource manager for loading 
	 * Ids will be hashed from the paths and used to link between materials
	 * and resources
	 */
	using ImageId = uint32_t;
	struct TextureInfo
	{
		Util::String path;
		ImageId id = UINT32_MAX;
	};

	struct TextureIds
	{
		TextureInfo baseColour;
		TextureInfo emissive;
		TextureInfo metallicRoughness;
		TextureInfo normal;
		TextureInfo occlusion;
	} textures;

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;
};

}    // namespace OmegaEngine
