#pragma once

#include "utility/String.h"

#include <cstdint>

namespace OmegaEngine
{

using ResourceHandle = uint64_t;

/**
* @brief All the information needed to build a vulkan texture
*/
struct TextureResource : public ResourceBase
{
	uint8_t level = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t depth = 1;		//< 3d textures not supported at present
	TextureFormat format = TextureFormat::FLOAT32;
	
};

/**
* @brief A buffer resource
*/
struct BufferResource : public ResourceBase
{
	size_t size;
	VulkanAPI::BufferType type;
};

struct ResourceBase
{
	enum class ResourceType
	{
		Texture,
		Buffer
	};

	virtual ~ResourceBase() = default;
	ResourceBase(const ResourceBase&) = delete;
	ResourceBase& operator=(const ResourceBase&) = delete;

	Util::String name;
	size_t index;
	ResourceType type;

};

class AttachmentInfo
{
public:


private:

	ResourceBase* resource;
};

struct Attachment
{
	AttachmentInfo colour;
	AttachmentInfo depth;
	uint8_t samples = 0;		//< For mult-sampling. Not used at present
};

}    // namespace OmegaEngine
