#pragma once

#include "utility/String.h"

#include <cstdint>

namespace OmegaEngine
{

using AttachmentHandle = uint64_t;
using ResourceHandle = uint64_t;

/**
* @brief All the information needed to build a vulkan texture
*/
struct TextureResource : public ResourceBase
{
	// the image information which will be used to create the image view
	uint8_t level = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t depth = 1;                                //< 3d textures not supported at present
	uint8_t samples = 0;                              //< For mult-sampling. Not used at present
	TextureFormat format = TextureFormat::FLOAT32;    //< The format will determine the type of attachment
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
	Util::String name;

	uint32_t reference = 0;

	// ==== variables set by the compiler =====
	// the number of passes this resource is being used as a input
	size_t inputCount = 0;

	// the renderpass that this resource is used as a output
	RenderPass* outputPass = nullptr;
};

struct AttachmentInfo
{
	// The type of resource associated with this attachment
	enum class ResourceType
	{
		Texture,
		Buffer
	};

	AttachmentInfo()
	{
	}
	~AttachmentInfo()
	{
	}

	AttachmentInfo(const AttachmentInfo&) = delete;
	AttachmentInfo& operator=(const AttachmentInfo&) = delete;

	Util::String name;
	size_t index;
	ResourceType type;

	// a handle to the resource data which is held by the graph
	ResourceHandle resource;
};


}    // namespace OmegaEngine
