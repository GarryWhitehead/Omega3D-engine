#include "TextureManager.h"

#include "DataTypes/TextureType.h"
#include "stb_image.h"

namespace OmegaEngine
{

	TextureManager::TextureManager()
	{
	}


	TextureManager::~TextureManager()
	{
	}

	void TextureManager::addGltfImage(tinygltf::Image& image)
	{
		MappedTexture mappedTex;
		int imageSize = image.width * image.height;

		mappedTex.loadPngTexture(imageSize, image.image.data());
		textures.insert(std::make_pair(image.name.c_str(), mappedTex));	
	}

	uint32_t TextureManager::findTexture(const char* name)
	{
		if (textures.find(name) != textures.end()) {
			auto iter = textures.find(name);
			
		}
	}

}