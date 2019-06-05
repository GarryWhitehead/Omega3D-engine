#include "MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Managers/ComponentInterface.h"
#include "OEMaths/OEMaths_transform.h"
#include "Vulkan/VkTextureManager.h"
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
