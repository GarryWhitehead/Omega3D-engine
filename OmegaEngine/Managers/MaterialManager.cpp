#include "MaterialManager.h"
#include "ObjectInterface/ComponentInterface.h"
#include "AssetInterface/AssetManager.h"
#include "OEMaths/OEMaths_transform.h"
#include "VulkanAPI/VkTextureManager.h"
#include "Models/ModelImage.h"
#include "Managers/EventManager.h"
#include "Engine/Omega_Global.h"

namespace OmegaEngine
{

	MaterialManager::MaterialManager()
	{
	}


	MaterialManager::~MaterialManager()
	{
	}



	MaterialInfo& MaterialManager::get(uint32_t index)
	{
		assert(index < materials.size());
		return materials[index];
	}

	void MaterialManager::addMaterial(std::unique_ptr<ModelMaterial>& material, std::vector<std::unique_ptr<ModelImage> >& images, std::unique_ptr<AssetManager>& assetManager)
	{
		MaterialInfo newMaterial;

		newMaterial.name = material->getName();

		// important that the name is valid as this is used to trace textures in the vulkan backend
		assert(!newMaterial.name.empty() || newMaterial.name != "");

		// like for like copy for the factors
		auto& factors = material->getFactors();
		newMaterial.factors.baseColour = factors.baseColour;
		newMaterial.factors.diffuse = factors.diffuse;
		newMaterial.factors.emissive = factors.emissive;
		newMaterial.factors.metallic = factors.metallic;
		newMaterial.factors.roughness = factors.roughness;
		newMaterial.factors.specular = factors.specular;
		newMaterial.factors.specularGlossiness = factors.specularGlossiness;

		// uv sets - straight copy
		auto& uvSets = material->getUvSets();
		newMaterial.uvSets.baseColour = uvSets.baseColour;
		newMaterial.uvSets.diffuse = uvSets.diffuse;
		newMaterial.uvSets.emissive = uvSets.emissive;
		newMaterial.uvSets.metallicRoughness = uvSets.metallicRoughness;
		newMaterial.uvSets.normal = uvSets.normal;
		newMaterial.uvSets.occlusion = uvSets.occlusion;
		newMaterial.uvSets.specularGlossiness = uvSets.specularGlossiness;

		// blending
		std::string alphaMaskStr = material->getAlphaMask();
		if (alphaMaskStr == "BLEND")
		{
			newMaterial.alphaMask = MaterialInfo::AlphaMode::Blend;
		}
		else if (alphaMaskStr == "MASK")
		{
			newMaterial.alphaMask = MaterialInfo::AlphaMode::Mask;
		}
		newMaterial.alphaMaskCutOff = material->getAlphaCutOff();

		// now sort the images associaed with this material -
		// add to the asset manager - they will be retrieved later through the material name + texture type
		for (uint32_t i = 0; i < (int)ModelMaterial::TextureId::Count; ++i)
		{
			auto id = material->getTexture(static_cast<ModelMaterial::TextureId>(i));
			if (id > 0)
			{
				assetManager->addImage(images[id], newMaterial.name + textureExtensions[i]);
				newMaterial.hasTexture[i] = true;
			}
			else
			{
				// load dummy texture?
			}
		}

		materials.emplace_back(newMaterial);
	}

	void MaterialManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty)
		{
			// not much to do yet....
			isDirty = false;
		}
	}
}
