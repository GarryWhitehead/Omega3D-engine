#include "RenderPass.h"

#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

RenderPass::RenderPass(VkContext& context)
    : device(context.getDevice())
{
}

RenderPass::~RenderPass()
{
    device.destroy(renderpass);
}

bool RenderPass::isDepth(const vk::Format format)
{
	std::vector<vk::Format> depthFormats = { vk::Format::eD16Unorm,       vk::Format::eX8D24UnormPack32,
		                                     vk::Format::eD32Sfloat,      vk::Format::eD16UnormS8Uint,
		                                     vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint };
	return std::find(depthFormats.begin(), depthFormats.end(), format) != std::end(depthFormats);
}

bool RenderPass::isStencil(const vk::Format format)
{
	std::vector<vk::Format> stencilFormats = { vk::Format::eS8Uint, vk::Format::eD16UnormS8Uint,
		                                       vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint };
	return std::find(stencilFormats.begin(), stencilFormats.end(), format) != std::end(stencilFormats);
}

vk::ImageLayout RenderPass::getFinalTransitionLayout(vk::Format format)
{
	vk::ImageLayout result;
	if (RenderPass::isStencil(format) || RenderPass::isDepth(format))
	{
		result = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
	}
	else
	{
		result = vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	return result;
}

void RenderPass::addOutputAttachment(const vk::Format format, const vk::ImageLayout initialLayout, const vk::ImageLayout finalLayout, const size_t reference, RenderPass::ClearFlags& clearFlags)
{
    // the description of this attachment
    vk::AttachmentDescription attachDescr;
    attachDescr.format = foramt;
    attachDescr.initialLayout = initalLayout;
    attachDescr.finalLayout = finalLayout;
    
    // clear flags
    attachDescr.loadOp = clearFlagsToVk(clearFlags.attachLoad);               // pre image state
    attachDescr.storeOp = clearFlagsToVk(clearFlags.attachStore);             // post image state
    attachDescr.stencilLoadOp = clearFlagsToVk(clearFlags.stencilLoad);       // pre stencil state
    attachDescr.stencilStoreOp = clearFlagsToVk(clearFlags.stencilStore);     // post stencil state
    
	vk::AttachmentDescription attachDescr(
	    {}, format, vk::SampleCountFlagBits::e1,
	    clearAttachment ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
	    clearAttachment ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare,
	    vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, finalLayout);
    
    attachments.emplace_back(attachDescr);
    
    // reference to the attachment
    OutputReferenceInfo info;
    info.ref.attachment = reference;
    info.ref.layout = finalLayout;
    info.index = attachments.size() - 1;
    
    outputRefs.emplace_back(ref);
}

void RenderPass::addInputRef(const uint32_t reference)
{
    // obtain the layout from the output ref - this also acts as a sanity check that a reader
    // has a writer
    vk::AttachmentReference ref;
    ref.attachment = reference;
    ref.layout = finalLayout;
    
    inputRefs.emplace_back(ref);
}

bool RenderPass::addSubPass(std::vector<uint32_t> inputRefs, std::vector<uint32_t>& outputRefs, uint32_t depthRef)
{
    assert(!colorRef.empty() || !inputRef.empty());

	// override default subpass with user specified subpass
    SubPassInfo subpass;
	subpass.descr.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;  // only graphic renderpasses supported at present
    
    // sort out the colour refs
    for (uint32_t& ref : outputRefs)
    {
        auto iter = attachments.find(ref);
        if (iter == attachments.end())
        {
            LOGGER_ERROR("Inavlid colour attachment reference.");
            return false;
        }
        
        subpass.colourRefs.emplace_back(*iter.second);
    }
    
    // and the input refs
    for (uint32_t& ref : inputRefs)
    {
        auto iter = attachments.find(ref);
        if (iter == attachments.end())
        {
            LOGGER_ERROR("Inavlid input attachment reference.");
            return false;
        }
        
        subpass.inputRefs.emplace_back(*iter.second);
    }
    
    // and the dpeth if required
    if (depthRef != UINT32_MAX)
    {
        auto iter = attachments.find(depthRef);
        if (iter == attachments.end())
        {
            LOGGER_ERROR("Inavlid depth attachment reference.");
            return false;
        }
        subpass.depth = &*iter.second;
    }
    
	subpass.descr.colorAttachmentCount = static_cast<uint32_t>(subpass.colourRefs.size());
	subpass.descr.pColorAttachments = subpass.colourRefs.data();

	// input attachments
	subpass.descr.inputAttachmentCount = static_cast<uint32_t>(subpass.inputRefs.size());
	subpass.descr.pInputAttachments = subpass.inputRefs.data();

	// depth attachment - if required
	if (depthRef != nullptr)
	{
		subpass.descr.pDepthStencilAttachment = subpass.depth;
	}

	subpasses.emplace_back(subpass);
}


void RenderPass::addSubpassDependency(DependencyTemplate dependencyTemplate, uint32_t srcSubpass, uint32_t dstSubpass)
{
	vk::SubpassDependency depend;

	if (dependencyTemplate == DependencyTemplate::Top_Of_Pipe)
	{
		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		depend.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		depend.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		depend.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
		depend.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	else if (dependencyTemplate == DependencyTemplate::Bottom_Of_Pipe)
	{
		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		depend.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		depend.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		depend.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	else if (dependencyTemplate == DependencyTemplate::Multi_Subpass)
	{
		depend.srcSubpass = srcSubpass;
		depend.dstSubpass = dstSubpass;
		depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		depend.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		depend.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	else if (dependencyTemplate == DependencyTemplate::Stencil_Subpass_Bottom)
	{
		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		depend.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		depend.dstStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
		depend.srcAccessMask = vk::AccessFlagBits::eShaderRead;
		depend.dstAccessMask =
		    vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	else if (dependencyTemplate == DependencyTemplate::Stencil_Subpass_Fragment)
	{
		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
		depend.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		depend.srcAccessMask =
		    vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	else if (dependencyTemplate == DependencyTemplate::DepthStencilSubpassTop)
	{
		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		depend.srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		depend.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		depend.srcAccessMask = vk::AccessFlagBits::eShaderRead;
		depend.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	else if (dependencyTemplate == DependencyTemplate::DepthStencilSubpassBottom)
	{
		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		depend.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		depend.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
	}
	dependency.push_back(depend);
}

void RenderPass::prepareRenderPass()
{
    // copy all the subpass declerations into one container
    std::vector<vk::SubpassDescription> descrs;
    for (SubpassInfo& subpass : subpasses)
    {
        descrs.emplace_back(subpass.descr);
    }
    
    assert(!descrs.empty);
    
	vk::RenderPassCreateInfo createInfo({}, static_cast<uint32_t>(attachment.size()), attachment.data(),
	                                    static_cast<uint32_t>(descrs.size()), descrs.data(),
	                                    static_cast<uint32_t>(dependency.size()), dependency.data());

	VK_CHECK_RESULT(device.createRenderPass(&createInfo, nullptr, &renderpass));
}

vk::RenderPassBeginInfo RenderPass::getBeginInfo(vk::ClearColorValue& backgroundColour, uint32_t index)
{
	// set up clear colour for each colour attachment
	clearValues.resize(attachment.size());
	for (uint32_t i = 0; i < attachment.size(); ++i)
	{
		if (attachment[i].finalLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal ||
		    attachment[i].finalLayout == vk::ImageLayout::eDepthStencilReadOnlyOptimal)
		{
			clearValues[i].depthStencil = { 1.0f, 0 };
		}
		else
		{
			clearValues[i].color = backgroundColour;
		}
	}

	vk::RenderPassBeginInfo beginInfo(renderpass, framebuffers[index], { { 0, 0 }, { imageWidth, imageHeight } },
	                                  static_cast<uint32_t>(clearValues.size()), clearValues.data());

	return beginInfo;
}

vk::RenderPassBeginInfo RenderPass::getBeginInfo(uint32_t size, vk::ClearValue* colour, uint32_t index)
{

	vk::RenderPassBeginInfo beginInfo(renderpass, framebuffers[index], { { 0, 0 }, { imageWidth, imageHeight } }, size,
	                                  colour);

	return beginInfo;
}

vk::RenderPassBeginInfo RenderPass::getBeginInfo(uint32_t index)
{
	// Don't clear - retain the attachments from the last pass

	vk::RenderPassBeginInfo beginInfo(renderpass, framebuffers[index], { { 0, 0 }, { imageWidth, imageHeight } }, 0,
	                                  nullptr);

	return beginInfo;
}

// ========================= frame buffer =================================

void FrameBuffer::prepare(RenderPass& rpass, std::vector<ImageView>& imageViews, uint32_t width, uint32_t height,
                                    uint32_t layerCount)
{
    assert(imageViews.size() > 0);

    // store locally the screen extents for use later
    this->width = width;
    this->height = height;

    vk::FramebufferCreateInfo frameInfo({}, rpass.get(), imageViews.size(), imageViews.data(), width, height,
                                        layerCount);

    VK_CHECK_RESULT(device.createFramebuffer(&frameInfo, nullptr, &framebuffer));
}


}    // namespace VulkanAPI
