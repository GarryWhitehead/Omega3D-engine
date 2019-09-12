#include "MaterialManager.h"
#include "AssetInterface/AssetManager.h"
#include "Engine/Omega_Global.h"
#include "Managers/EventManager.h"
#include "Models/ModelImage.h"
#include "OEMaths/OEMaths_transform.h"
#include "ObjectInterface/ComponentInterface.h"
#include "VulkanAPI/VkTextureManager.h"

namespace OmegaEngine
{

MaterialManager::MaterialManager()
{
}

MaterialManager::~MaterialManager()
{
}

MaterialInfo &MaterialManager::getMaterial(uint32_t index)
{
	assert(index < materials.size());
	return materials[index];
}

void MaterialManager::addComponentToManager(MaterialComponent *component)
{
	MaterialInfo newMaterial;
	newMaterial.name = component->name;

	// important that the name is valid as this is used to trace textures in the vulkan backend
	assert(!newMaterial.name.empty() || newMaterial.name != "");

	newMaterial.factors.baseColour = component->baseColour;
	newMaterial.factors.diffuse = OEMaths::vec4f(component->diffuse, 1.0f);
	newMaterial.factors.emissive = component->emissive;
	newMaterial.factors.metallic = component->metallic;
	newMaterial.factors.roughness = component->roughness;
	newMaterial.factors.specular = component->specular;

	// only opaque supported for user-defined materials at the moment
	newMaterial.alphaMask = MaterialInfo::AlphaMode::Opaque;

	// TODO : this needs looking at, possiblr decoupling from materials. Textures should be a separate component and linked via name
	// or offset. For now, just use dummy textures
	for (uint32_t i = 0; i < (int)ModelMaterial::TextureId::Count; ++i)
	{
		
			MappedTexture dummyTexture;
			dummyTexture.createEmptyTexture(1024, 1024, TextureFormat::Image8UC4, true);
		    std::string matId = AssetManager::materialIdentifier + newMaterial.name + '_' +
		                        std::get<0>(textureExtensions[i]);

		    AssetImageUpdateEvent event{ matId, dummyTexture };
		    Global::eventManager()->instantNotification(std::move(event));
	}

	materials.emplace_back(newMaterial);
	component->offset = materials.size() - 1;
}

void MaterialManager::addMaterial(std::unique_ptr<ModelMaterial> &material,
                                  std::vector<std::unique_ptr<ModelImage>> &images)
{
	MaterialInfo newMaterial;

	newMaterial.name = material->name;

	// important that the name is valid as this is used to trace textures in the vulkan backend
	assert(!newMaterial.name.empty() || newMaterial.name != "");

	// like for like copy for the factors
	auto &factors = material->factors;
	newMaterial.factors.baseColour = factors.baseColour;
	newMaterial.factors.diffuse = factors.diffuse;
	newMaterial.factors.emissive = factors.emissive;
	newMaterial.factors.metallic = factors.metallic;
	newMaterial.factors.roughness = factors.roughness;
	newMaterial.factors.specular = factors.specular;
	newMaterial.factors.specularGlossiness = factors.specularGlossiness;

	// uv sets - straight copy
	auto &uvSets = material->uvSets;
	newMaterial.uvSets.baseColour = uvSets.baseColour;
	newMaterial.uvSets.diffuse = uvSets.diffuse;
	newMaterial.uvSets.emissive = uvSets.emissive;
	newMaterial.uvSets.metallicRoughness = uvSets.metallicRoughness;
	newMaterial.uvSets.normal = uvSets.normal;
	newMaterial.uvSets.occlusion = uvSets.occlusion;
	newMaterial.uvSets.specularGlossiness = uvSets.specularGlossiness;

	// blending
	std::string alphaMaskStr = material->factors.mask;
	if (alphaMaskStr == "BLEND")
	{
		newMaterial.alphaMask = MaterialInfo::AlphaMode::Blend;
	}
	else if (alphaMaskStr == "MASK")
	{
		newMaterial.alphaMask = MaterialInfo::AlphaMode::Mask;
	}
	else
	{
		newMaterial.alphaMask = MaterialInfo::AlphaMode::Opaque;
	}

	newMaterial.alphaMaskCutOff = material->factors.alphaMaskCutOff;

	newMaterial.usingSpecularGlossiness = material->usingSpecularGlossiness;

	// now sort the images associaed with this material -
	// add to the asset manager - they will be retrieved later through the material name + texture type
	// the "GROUPED" identifier is used to group these texture together in the Vulkan texture manager
	assert(textureExtensions.size() == (int)ModelMaterial::TextureId::Count);

	for (uint32_t i = 0; i < (int)ModelMaterial::TextureId::Count; ++i)
	{
		auto id = material->getTexture(static_cast<ModelMaterial::TextureId>(i));
		if (id > -1)
		{
			std::string matId = AssetManager::materialIdentifier + newMaterial.name +
			                                       '_' + std::get<0>(textureExtensions[i]);
			
			AssetGltfImageUpdateEvent event{ matId, std::move(images[id]) };
			Global::eventManager()->instantNotification(event);

			newMaterial.hasTexture[i] = true;
		}
		else
		{
			MappedTexture dummyTexture;
			dummyTexture.createEmptyTexture(1024, 1024, TextureFormat::Image8UC4, true);
			std::string matId = AssetManager::materialIdentifier +
			                                         newMaterial.name + '_' +
			                                         std::get<0>(textureExtensions[i]);

			AssetImageUpdateEvent event{ matId, dummyTexture };
			Global::eventManager()->instantNotification(std::move(event));
		}
	}

	materials.emplace_back(newMaterial);
}

void MaterialManager::updateFrame(double time, double dt,
                                  std::unique_ptr<ObjectManager> &objectManager,
                                  ComponentInterface *componentInterface)
{
	if (isDirty)
	{
		// not much to do yet....
		isDirty = false;
	}
}
} // namespace OmegaEngine
