#include "TextureManager.h"
#include "DataTypes/TextureType.h"
#include "Utility/logger.h"
#include "Vulkan/Sampler.h"
#include "Omega_Common.h"
#include "Objects/ObjectManager.h"
#include "Managers/ComponentInterface.h"

namespace OmegaEngine
{

	TextureManager::TextureManager()
	{
		// this won't be used, just required to keep vulkan happy
		dummyTexture.createEmptyTexture(1024, 1024, vk::Format::eR8G8B8A8Unorm, true);
	}


	TextureManager::~TextureManager()
	{
	}

	void TextureManager::updateFrame(double time, double dt,
		std::unique_ptr<ObjectManager>& objectManager,
		ComponentInterface* componentInterface)
	{

	}

	

	uint32_t TextureManager::getTextureIndex(uint32_t set, const char* name)
	{
		for (uint32_t i = 0; i < textures.size(); ++i) 
		{
			if (strcmp(name, textures[set][i].getName()) == 0) 
			{
				return i;
			}
		}
		return UINT32_MAX;
	}

	MappedTexture& TextureManager::getTexture(uint32_t set, int index)
	{
		assert(index < textures[set].size());
		return textures[set][index];
	}

	VulkanAPI::SamplerType TextureManager::getSampler(uint32_t set, uint32_t index)
	{
		// not all gltf files speciify sampler types so go with a default otherwise
		if (samplers[set].empty()) 
		{
			// if no samplers defined then go with this as default for now
			// TODO: Add option in shader reflection to add samplers based on image name
			return VulkanAPI::SamplerType::Clamp;	
		}
		return samplers[set][index];
	}

	VulkanAPI::SamplerType TextureManager::getDummySampler()
	{
		// this could be anything as its not used
		return VulkanAPI::SamplerType::Clamp;
	}

}