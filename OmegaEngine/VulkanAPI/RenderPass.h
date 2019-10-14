#pragma once

#include "VulkanAPI/Common.h"

#include "VulkanAPI/VkContext.h"

#include <assert.h>
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace VulkanAPI
{

// forward declerations
class ImageView;

class RenderPass
{

public:
    
    enum class ClearType
    {
        Store,
        Clear,
        DontCare
    };
    
    /**
     * Describes what should be done with the images pre- and post- pass - i.e. keep or throw away the data
     */
    ClearFlags
    {
        ClearType attachLoad = ClearType::DontCare;
        ClearType attachStore = ClearType::DontCare;
        ClearType stencilLoad = ClearType::DontCare;
        ClearType stencilStore = ClearType::DontCare;
    };
    
	RenderPass(VkContext& context);
	~RenderPass();
    
    // no copying
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    // static functions
	static vk::ImageLayout getFinalTransitionLayout(const vk::Format format);
	static bool isDepth(const vk::Format format);
	static bool isStencil(const vk::Format format);

	vk::RenderPass get()
	{
		return renderpass;
	}
    
    // Adds a attahment, including reference for this pass. This can be a colour, input or depth attachment - what this will be used as will be deferred by **addSubpass**
    // Note: if the format is a depth/stencil format, then this will be added as a depth attachment
	void addAttachment(const vk::Format format, const vk::ImageLayout initialLayout, const vk::ImageLayout finalLayout, const size_t reference, ClearFlags clearFlags);
    
    // Adds a subpass, the colour outputs and inputs will be linked via the reference ids. These must have already been added as attachments, otherwise this will throw an error
	bool addSubPass(std::vector<uint32_t> inputRefs, std::vector<uint32_t>& outputRefs, uint32_t depthRef = UINT32_MAX);

    
	void addSubpassDependency(DependencyTemplate dependencyTemplate, uint32_t srcSubpass = 0,
	                          uint32_t dstSubpass = 0);    // templated version
	
    // Actually creates the renderpass based on the above definitions
    void prepare();

	// for generating cmd buffer
	vk::RenderPassBeginInfo getBeginInfo(vk::ClearColorValue& backgroundColour, uint32_t index = 0);
	vk::RenderPassBeginInfo getBeginInfo(uint32_t size, vk::ClearValue* backgroundColour, uint32_t index = 0);
	vk::RenderPassBeginInfo getBeginInfo(uint32_t index = 0);

private:
    
    struct SubpassInfo
    {
        vk::SubpassDescription descr;
        std::vector<vk::AttachmentReference> colourRefs;
        std::vector<vk::AttachmentReference> inputRefs;
        vk::AttachmentReference* depth = nullptr;
    };
    
private:
    // keep a refernece of the device this pass was created on for destruction purposes
	vk::Device device;

	vk::RenderPass renderpass;
    
    using Attachment = std::pair<vk::AttachmentDescription, vk::AttachmentReference>;
    
    // the colour/input attachments
	std::unordered_map<uint32_t, Attachment> attachments;
    
    // subpasses - could be a single or multipass
	std::vector<vk::SubpassDescription> subpasses;
    
    // the dependencies between renderpasses and external sources
	std::vector<vk::SubpassDependency> dependencies;
    
    // the depth attachment - if set, as indicated by the flag
    bool hasDepth = false;
    Attachment depthRef;

};

class FrameBuffer
{
public:

    FrameBuffer(VkContext& context) :
        device(context.getDevice())
    {}
    
    ~FrameBuffer()
    {
        device.destroy(framebuffer);
    }
    
    void prepare(RenderPass& rpass, std::vector<ImageView>& imageViews, uint32_t width, uint32_t height,
                 uint32_t layerCount);

private:
    
    // references
    vk::Device device;
    
    uint32_t width = 0;
    uint32_t height = 0;

    vk::Framebuffer framebuffer;
};

}    // namespace VulkanAPI
