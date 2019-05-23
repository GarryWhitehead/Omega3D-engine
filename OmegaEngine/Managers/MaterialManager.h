#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"

#include "Managers/DataTypes/TextureType.h"

#include "tiny_gltf.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{
	// forward declerations
	class TextureManager;

	enum class PbrMaterials 
	{
		BaseColor,
		Normal,
		MetallicRoughness,
		Emissive,
		Occlusion,
		Count
	};

	struct MaterialInfo
	{
		const char *name;

		enum class AlphaMode
		{
			Opaque,
			Blend,
			Mask
		};

		struct Texture
		{
			uint32_t set;
			uint32_t sampler = 0;
			uint32_t image = 0;			// set number and the index within this set
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

			AlphaMode alphaMask = AlphaMode::Opaque;
			float alphaMaskCutOff = 1.0f;
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

		// material image indicies
		std::array<Texture, static_cast<int>(PbrMaterials::Count)> textures;
		std::array<bool, static_cast<int>(PbrMaterials::Count)> textureState = { false };

		// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
		bool usingSpecularGlossiness = false;
	};

	class MaterialManager : public ManagerBase
	{

	public:

		MaterialManager();
		~MaterialManager();

		// a per-frame update if the material data becomes dirty
		void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface) override;

		void addGltfMaterial(uint32_t set, tinygltf::Material& gltf_mat, TextureManager& textureManager);
		MaterialInfo& get(uint32_t index);

	private:
		
		std::vector<MaterialInfo> materials;
		bool isDirty = true;

	};

}

