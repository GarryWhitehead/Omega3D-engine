#include "VkTextureManager.h"
#include "Vulkan/DataTypes/Texture.h"

namespace VulkanAPI
{

	VkTextureManager::VkTextureManager()
	{
	}


	VkTextureManager::~VkTextureManager()
	{
	}

	Texture VkTextureManager::getTexture(TextureHandle handle)
	{
		auto iter = textures.find(std::get<0>(handle));

		// make sure this texture type exsists
		if (iter == textures.end()) {
			// return dummy texture
			Texture tex;
			return tex;
		}

		return iter->second[std::get<1>(handle)];
	}
	
	
}