#pragma once
#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"
#include "AssetInterface/MappedTexture.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{
	// forward declerations
	class TextureManager;
	class ModelMaterial;
	class ModelImage;
	class AssetManager;

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

		// if using specular glossiness then color and metallic/roughness texture indicies will be automatically changed for this workflow
		bool usingSpecularGlossiness = false;
	};

	class MaterialManager : public ManagerBase
	{

	public:

		// this must be in the same order as the model material texture enum
		std::array<std::string, 5> textureExtensions
		{
			"_BaseColour",
			"_Emissive",
			"_MetallicRoughness",
			"_Normal",
			"_Occlusion"
		};

		MaterialManager();
		~MaterialManager();

		// a per-frame update if the material data becomes dirty
		void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface) override;

		void addMaterial(std::unique_ptr<ModelMaterial>& material, std::vector<std::unique_ptr<ModelImage> >& images, std::unique_ptr<AssetManager>& assetManager);
		MaterialInfo& get(uint32_t index);

		uint32_t getBufferOffset() const
		{
			return materials.size();
		}

	private:
		
		std::vector<MaterialInfo> materials;
		bool isDirty = true;

	};

}

