#include "TextureManager.h"
#include "Vulkan/DataTypes/Texture.h"

namespace VulkanAPI
{

	TextureManager::TextureManager()
	{
	}


	TextureManager::~TextureManager()
	{
	}

	Texture& TextureManager::getTexture(TextureHandle handle)
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