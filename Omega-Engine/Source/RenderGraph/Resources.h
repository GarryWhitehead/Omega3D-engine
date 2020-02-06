#pragma once

#include "utility/CString.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/RenderPass.h"

#include <cstddef>
#include <cstdint>

// vulkan forward declerations
namespace VulkanAPI
{
class ImageView;
class Texture;
class VkDriver;
}

namespace OmegaEngine
{

// forward declerations
class RenderGraphPass;
class RenderGraph;

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
    
    // abstract virtual function
    virtual void* bake(VulkanAPI::VkDriver& driver) = 0;
    
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
    TextureResource(const uint32_t width, const uint32_t height, const vk::Format format, const uint8_t level, const uint8_t layers);
    
    void* bake(VulkanAPI::VkDriver& driver) override;
    VulkanAPI::Texture* get();
    
    bool isDepthFormat();
    bool isColourFormat();
    bool isStencilFormat();
    
	// the image information which will be used to create the image view
    uint8_t samples = 1;
    uint32_t width = 0;
	uint32_t height = 0;
	uint8_t layers = 1;                            //< 3d textures not supported at present
	uint8_t level = 0;                             //< For mult-sampling. Not used at present
	vk::Format format = vk::Format::eUndefined;    //< The format will determine the type of attachment
    vk::ImageUsageFlagBits imageLayout;
    
    // used by the vulkan backend
    std::unique_ptr<VulkanAPI::Texture> texture;
    
    VulkanAPI::RenderPass::ClearFlags clearFlags;
};

/**
* @brief A buffer resource
*/
struct BufferResource : public ResourceBase
{
	size_t size;
	VulkanAPI::Buffer::Usage usage;
};

struct AttachmentInfo
{
    AttachmentInfo() = default;

	// creates the 'actual' vulkan resource associated with this attachment
	void* bake(VulkanAPI::VkDriver& driver, RenderGraph& rGraph);

	Util::String name;
	size_t index;
	uint8_t samples = 0;

	// a handle to the resource data which is held by the graph
	ResourceHandle resource;
};


}    // namespace OmegaEngine
