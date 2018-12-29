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

		void addGltfImage(tinygltf::Image& image);
		uint32_t findTexture(const char* name);

	private:

		std::unordered_map<const char*, MappedTexture> textures;
	};

}

