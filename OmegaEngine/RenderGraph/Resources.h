#pragma once

#include "utility/CString.h"

#include "VulkanAPI/common.h"

#include <cstddef>
#include <cstdint>

namespace OmegaEngine
{

// forward declerations
class RenderGraphPass;

using AttachmentHandle = uint64_t;
using ResourceHandle = uint64_t;

struct ResourceBase
{
	enum class ResourceType
	{
		Texture,
		Buffer
	};

	virtual ~ResourceBase() = default;
	ResourceBase(const ResourceType rtype)
	    : type(rtype)
	{
	}

	Util::String name;
	ResourceType type;

	// ==== set by the compiler =====
	// the number of passes this resource is being used as a input
	size_t inputCount = 0;

	// the reference count after culling
	size_t refCount = 0;

	// used by the attachment descriptor
	uint32_t referenceId = 0;

	// the renderpass that this resource is used as a output
	RenderGraphPass* outputPass = nullptr;
};

/**
* @brief All the information needed to build a vulkan texture
*/
struct TextureResource : public ResourceBase
{
	TextureResource(const uint32_t width, const uint32_t height, const vk::Format format, const uint8_t level,
	                const uint8_t layers)
	    : width(width)
	    , height(height)
	    , layers(layers)
	    , level(level)
	    , format(format)
	    , ResourceBase(ResourceType::Texture)
	{
	}
	// the image information which will be used to create the image view
	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t layers = 1;                            //< 3d textures not supported at present
	uint8_t level = 0;                             //< For mult-sampling. Not used at present
	vk::Format format = vk::Format::eUndefined;    //< The format will determine the type of attachment
};

/**
* @brief A buffer resource
*/
struct BufferResource : public ResourceBase
{
	size_t size;
	VulkanAPI::BufferType type;
};

struct AttachmentInfo
{
	AttachmentInfo()
	{
	}
	~AttachmentInfo()
	{
	}

	AttachmentInfo(const AttachmentInfo&) = delete;
	AttachmentInfo& operator=(const AttachmentInfo&) = delete;

	// finialises the attachment
	void prepare();

	// creates the 'actual' vulkan resource associated with this attachment
	void bake();

	Util::String name;
	size_t index;
	uint8_t samples = 0;

	// a handle to the resource data which is held by the graph
	ResourceHandle resource;
};


}    // namespace OmegaEngine
