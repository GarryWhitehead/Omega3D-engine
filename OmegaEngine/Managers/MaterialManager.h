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
class TextureManager;
class ModelImage;

struct MaterialInfo
{
	enum class AlphaMode
	{
		Opaque,
		Blend,
		Mask
	};

	struct Factors
	{
		OEMaths::vec3f emissive = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f };
		OEMaths::vec4f baseColour = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
		OEMaths::vec4f diffuse = OEMaths::vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
		OEMaths::vec3f specular = OEMaths::vec3f{ 0.0f, 0.0f, 0.0f };

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

	// This must not be empty as used for identifying textues when passed to the vulkan api
	std::string name;

	AlphaMode alphaMask = AlphaMode::Opaque;
	float alphaMaskCutOff = 1.0f;

	std::array<bool, (int)ModelMaterial::TextureId::Count> hasTexture = { false };

	// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
	bool usingSpecularGlossiness = false;
};

class MaterialManager
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

	MaterialManager();
	~MaterialManager();

	void addComponentToManager(MaterialComponent *component);

	void addMaterial(std::unique_ptr<ModelMaterial> &material,
	                 std::vector<std::unique_ptr<ModelImage>> &images);
	MaterialInfo &get(uint32_t index);

	uint32_t getBufferOffset() const
	{
		return static_cast<uint32_t>(materials.size());
	}

private:
	std::vector<MaterialInfo> materials;
	bool isDirty = true;
};

} // namespace OmegaEngine
