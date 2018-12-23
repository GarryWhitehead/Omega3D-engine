#pragma once

#include <vector>
#include <unordered_map>
#include "tiny_gltf.h"

// forward declerations
class MappedTexture;

namespace OmegaEngine
{
	

	class TextureManager
	{
	public:

		TextureManager();

		~TextureManager();

	private:

		std::unordered_map<int, MappedTexture> textures;
	};

}

