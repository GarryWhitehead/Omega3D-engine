#pragma once
#include "Vulkan/Common.h"

#include <unordered_map>
#include <tuple>

namespace VulkanAPI
{
	// forward declerations
	enum class TextureType;
	class Texture;

	using TextureHandle = std::tuple<TextureType, uint32_t>;

	class TextureManager
	{

	public:
		
		
		TextureManager();
		~TextureManager();

		Texture& getTexture(TextureHandle handle);

	private:

		std::unordered_map<TextureType, std::vector<Texture> > textures;
	};

}

