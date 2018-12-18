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


	void TextureManager::parseGltfTextures(tinygltf::Model& model)
	{
		for (const auto texture : model.textures) {

			MappedTexture mappedTex;
			const auto& image = model.images[texture.source];
			int imageSize = image.width * image.height;
			
			mappedTex.loadPngTexture(imageSize, image.image.data());
			textures.insert(std::make_pair(image.name.c_str(), mappedTex));
		}
	}

}