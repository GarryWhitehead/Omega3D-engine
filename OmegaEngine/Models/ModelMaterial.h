#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

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

private:
	Util::String name;

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
	 */
	struct Textures
	{
		Util::String baseColour;
		Util::String emissive;
		Util::String metallicRoughness;
		Util::String normal;
		Util::String occlusion;
	} textures;

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;
};

}    // namespace OmegaEngine
