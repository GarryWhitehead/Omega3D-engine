#pragma once
#include "Types/MappedTexture.h"

#include "Models/ModelMaterial.h"

#include "OEMaths/OEMaths.h"

#include <array>
#include <memory>
#include <tuple>
#include <unordered_map>

namespace OmegaEngine
{
// forward declerations
class ModelImage;

class Material
{

public:

	enum class AlphaMode
	{
		Opaque,
		Blend,
		Mask
	};

	// this must be in the same order as the model material texture enum -
	// includes the binding number as per the model shader
	const std::vector<std::tuple<std::string, uint32_t>> textureExtensions = {
		{ "BaseColour", 0 },
		{ "Emissive", 3 },
		{ "MetallicRoughness", 2 },
		{ "Normal", 1 },
		{ "Occlusion", 4 }
	};

	Material();
	~Material();

	void prepare(MaterialComponent *component);

	void prepare(ModelMaterial& material);


private:
	
	// This must not be empty as used for identifying textues when passed to the vulkan api
	std::string name;

	AlphaMode alphaMask = AlphaMode::Opaque;
	float alphaMaskCutOff = 1.0f;

	std::array<bool, (int)ModelMaterial::TextureId::Count> hasTexture = { false };

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;

	struct Factors
	{
		OEMaths::vec3f emissive = OEMaths::vec3f{ 0.0f };
		OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f };
		OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f };
		OEMaths::vec3f specular = OEMaths::vec3f{ 0.0f };

		float specularGlossiness = 1.0f;
		float roughness = 1.0f;
		float metallic = 1.0f;
	} factors;

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

};

} // namespace OmegaEngine