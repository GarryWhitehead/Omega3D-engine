#include "MaterialManager.h"
#include "Managers/TextureManager.h"
#include "ObjectInterface/ComponentInterface.h"
#include "AssetInterface/AssetManager.h"
#include "OEMaths/OEMaths_transform.h"
#include "Vulkan/VkTextureManager.h"
#include "Models/ModelMaterial.h"
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

		newMaterial.name = material.name;

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
			assetManager->addImage(images[id], newMaterial.name + textureExtensions[i]);
		}

		materials.emplace_back(newMaterial);
	}

	void MaterialManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty) 
		{
			auto& textureManager = componentInterface->getManager<TextureManager>();

			for (auto& mat : materials) 
			{
				// all textures are going to be copied over to the the grpahics side
				for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {

					// do we actually have an image for this particular pbr material
					if (mat.textureState[i]) 
					{
						VulkanAPI::MaterialTextureUpdateEvent event{ mat.name, i, &textureManager.getTexture(mat.textures[i].set, mat.textures[i].image),
							textureManager.getSampler(mat.textures[i].set, mat.textures[i].sampler) };
						Global::eventManager()->addQueueEvent<VulkanAPI::MaterialTextureUpdateEvent>(event);
					}
					else 
					{
						VulkanAPI::MaterialTextureUpdateEvent event{ mat.name, i, &textureManager.getDummyTexture(), textureManager.getDummySampler() };
						Global::eventManager()->addQueueEvent<VulkanAPI::MaterialTextureUpdateEvent>(event);
					}
				}
			}
		}

		isDirty = false;
	}
}
