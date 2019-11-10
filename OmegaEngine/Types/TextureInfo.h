#pragma once

#include "Utility/String.h"

#include "Types/MappedTexture.h"

#include <cstdint>
#include <utility>

namespace VulkanAPI
{
class VkTextureManager;
}

namespace OmegaEngine
{

/**
* @brief Used to add textures to the vulkan back-end were they will be hosted on the GPU.
* This is for grouped textures were many textures are linked under the same id. This is
* useful for materials for instnace which have many elements described in the same descriptor set
*/
class GroupedTextureInfo
{
public:
	GroupedTextureInfo(Util::String _id, uint32_t _binding, OmegaEngine::MappedTexture* _mapped, SamplerType _sampler)
	    : id(_id)
	    , binding(_binding)
	    , texture(_mapped)
	    , sampler(_sampler)
	{
	}

	// no copy
	GroupedTextureInfo(GroupedTextureInfo&) = delete;
	GroupedTextureInfo& operator=(GroupedTextureInfo&) = delete;

	// but moving allowed
	GroupedTextureInfo(GroupedTextureInfo&& rhs)
	    : id(rhs.id)
	    , binding(std::exchange(rhs.binding, 0))
	    , texture(std::exchange(rhs.texture, nullptr))
	    , sampler(rhs.sampler)
	{
	}

	GroupedTextureInfo& operator=(GroupedTextureInfo&& rhs)
	{
		if (this != &rhs)
		{
			id = rhs.id;
			binding = std::exchange(rhs.binding, 0);
			texture = std::exchange(rhs.texture, nullptr);
			sampler = rhs.sampler;
		}
		return *this;
	}

	friend class VulkanAPI::VkTextureManager;

private:
	Util::String id;
	uint32_t binding = 0;
	OmegaEngine::MappedTexture* texture = nullptr;
	SamplerType sampler;
};

/**
* @brief Similiar to the above but for single textures.
*/
class TextureInfo
{
public:

	TextureInfo(Util::String _id, OmegaEngine::MappedTexture* _tex)
	    : id(_id)
	    , texture(_tex)
	{
	}

	// no copy
	TextureInfo(TextureInfo&) = delete;
	TextureInfo& operator=(TextureInfo&) = delete;

	// but moving allowed
	TextureInfo(TextureInfo&& rhs)
	    : id(rhs.id)
	    , texture(std::exchange(rhs.texture, nullptr))
	    , sampler(rhs.sampler)
	{
	}

	TextureInfo& operator=(TextureInfo&& rhs)
	{
		if (this != &rhs)
		{
			id = rhs.id;
			texture = std::exchange(rhs.texture, nullptr);
			sampler = rhs.sampler;
		}
		return *this;
	}

	friend class VulkanAPI::VkTextureManager;

private:

	Util::String id;
	OmegaEngine::MappedTexture* texture = nullptr;
	Sampler sampler;
};

}    // namespace OmegaEngine
