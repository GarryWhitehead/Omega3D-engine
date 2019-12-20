#include "RenderPass.h"

#include "VulkanAPI/VkContext.h"

#include "utility/Logger.h"

#include <assert.h>

namespace VulkanAPI
{

RenderPass::RenderPass(VkContext& context)
    : context(context)
{
}

RenderPass::~RenderPass()
{
	context.getDevice().destroy(renderpass, nullptr);
}


vk::AttachmentLoadOp RenderPass::clearFlagsToVk(const ClearType flags)
{
	vk::AttachmentLoadOp result;
	switch (flags)
	{
	case ClearType::Clear:
		result = vk::AttachmentLoadOp::eClear;
		break;
	case ClearType::DontCare:
		result = vk::AttachmentLoadOp::eDontCare;
		break;
	case ClearType::Store:
		result = vk::AttachmentLoadOp::eLoad;
		break;
	}
	return result;
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

void RenderPass::addOutputAttachment(const vk::Format format, const vk::ImageLayout initialLayout,
                                     const vk::ImageLayout finalLayout, const size_t reference, ClearFlags& clearFlags,
                                     uint32_t sampleCount)
{
	// the description of this attachment
	vk::AttachmentDescription attachDescr;
	attachDescr.format = format;
	attachDescr.initialLayout = initialLayout;
	attachDescr.finalLayout = finalLayout;

	// samples
	attachDescr.samples = samplesToVk(sampleCount);

	// clear flags
	attachDescr.loadOp = clearFlagsToVk(clearFlags.attachLoad);              // pre image state
	attachDescr.storeOp = clearFlagsToVk(clearFlags.attachStore);            // post image state
	attachDescr.stencilLoadOp = clearFlagsToVk(clearFlags.stencilLoad);      // pre stencil state
	attachDescr.stencilStoreOp = clearFlagsToVk(clearFlags.stencilStore);    // post stencil state

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

	outputRefs.emplace_back(info);
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

void RenderPass::addOutputRef(const uint32_t reference)
{
	vk::AttachmentReference ref;
	ref.attachment = reference;
	ref.layout = ;

	outputRefs.emplace_back(ref);
}

bool RenderPass::addSubPass(std::vector<uint32_t> inputRefs, std::vector<uint32_t>& outputRefs, uint32_t depthRef)
{
	// override default subpass with user specified subpass
    SubpassInfo subpass;
	subpass.descr.pipelineBindPoint =
	    vk::PipelineBindPoint::eGraphics;    // only graphic renderpasses supported at present

	// sort out the colour refs
	for (uint32_t& ref : outputRefs)
	{
        auto iter = std::find(attachments.begin(), attachments.end(), [&ref](const vk::AttachmentReference& lhs)
                  { return lhs.attachment == ref; });
        
		if (iter == attachments.end())
		{
			LOGGER_ERROR("Inavlid colour attachment reference.");
			return false;
		}

		subpass.colourRefs.emplace_back(iter);
	}

	// and the input refs
	for (uint32_t& ref : inputRefs)
	{
		auto iter = std::find(attachments.begin(), attachments.end(), [&ref](const vk::AttachmentReference& lhs)
        { return lhs.attachment == ref; });
        
		if (iter == attachments.end())
		{
			LOGGER_ERROR("Inavlid input attachment reference.");
			return false;
		}

		subpass.inputRefs.emplace_back(iter);
	}

	// and the dpeth if required
	if (depthRef != UINT32_MAX)
	{
		auto iter = std::find(attachments.begin(), attachments.end(), [&depthRef](const vk::AttachmentReference& lhs)
        { return lhs.attachment == depthRef; });
        
		if (iter == attachments.end())
		{
			LOGGER_ERROR("Inavlid depth attachment reference.");
			return false;
		}
		subpass.depth = iter;
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

void RenderPass::addSubpassDependency(uint32_t subpassRef, const Util::BitSetEnum<SubpassFlags>& flags)
{
	vk::SubpassDependency depend;
    depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
    
    if (flags.testBit(SubpassFlags::TopOfPipeline))
    {
        if (!flags.testBit(SubpassFlags::Merged))
        {
            depend.srcSubpass = VK_SUBPASS_EXTERNAL;
            depend.dstSubpass = 0;
        }
        
        if (flags.testBit(SubpassFlags::ColourRead))
        {
            depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            depend.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            depend.srcAccessMask = 0;
            depend.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        }
        else if (flags.testBit(SubpassFlags::DepthRead) && flags.testBit(SubpassFlags::StencilRead))
        {
            depend.srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
            depend.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            depend.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            depend.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
        else if (flags.testBit(SubpassFlags::StencilRead))
        {
            depend.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
            depend.dstStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
            depend.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            depend.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        }
    }
    else if (flags.testBit(SubpassFlags::BottomOfPipeline))
    {
        if (!flags.testBit(SubpassFlags::Merged))
        {
            depend.srcSubpass = 0;
            depend.dstSubpass = VK_SUBPASS_EXTERNAL;
        }
    }
	if (dependencyTemplate == DependencyTemplate::Top_Of_Pipe)
	{
		
		
		
	}
	else if (dependencyTemplate == DependencyTemplate::Bottom_Of_Pipe)
	{
		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		depend.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
		depend.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		depend.dstAccessMask = 0;
	}
	else if (dependencyTemplate == DependencyTemplate::Multi_Subpass)
	{
		depend.srcSubpass = srcSubpass;
		depend.dstSubpass = dstSubpass;
		depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		depend.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		depend.srcAccessMask = 0;
		depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	}
	else if (dependencyTemplate == DependencyTemplate::Stencil_Subpass_Bottom)
	{
		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		
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
	}
	else if (dependencyTemplate == DependencyTemplate::DepthStencilSubpassTop)
	{
		
	}
	else if (dependencyTemplate == DependencyTemplate::DepthStencilSubpassBottom)
	{
		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		depend.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		depend.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	}
	dependency.push_back(depend);
}

void RenderPass::prepare()
{
	// copy all the subpass declerations into one container
	std::vector<vk::SubpassDescription> descrs;
	for (const SubpassInfo& subpass : subpasses)
	{
		descrs.emplace_back(subpass.descr);
	}

	assert(!descrs.empty);

	vk::RenderPassCreateInfo createInfo({}, static_cast<uint32_t>(attachment.size()), attachment.data(),
	                                    static_cast<uint32_t>(descrs.size()), descrs.data(),
	                                    static_cast<uint32_t>(dependency.size()), dependency.data());

	VK_CHECK_RESULT(context.getDevice().createRenderPass(&createInfo, nullptr, &renderpass));
}

vk::RenderPass& RenderPass::get()
{
	return renderpass;
}

void RenderPass::setClearColour(OEMaths::colour4& col)
{
	clearCol = col;
}

void RenderPass::setDepthClear(float col)
{
	depthClear = col;
}

bool RenderPass::hasColourAttach()
{
	return !attachments.empty();
}

bool RenderPass::hasDepthAttach()
{
	return depthAttach;
}

std::vector<vk::PipelineColorBlendAttachmentState> RenderPass::getColourAttachs()
{
	size_t attachCount = attachments.size();
	assert(attachCount > 0);
	std::vector<vk::PipelineColorBlendAttachmentState> colAttachs(attachCount);
	
	// for each clear output colour attachment in the renderpass, we need a blend attachment
	for (uint32_t i = 0; i < attachments.size(); ++i)
	{
		vk::PipelineColorBlendAttachmentState colour;
		colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		                        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colour.blendEnable = blendFactor;
		colAttachs.push_back(colour);
	}
	return colAttachs;
}

// ========================= frame buffer =================================
FrameBuffer::FrameBuffer(VkContext& context)
    : device(context.getDevice())
{
}

FrameBuffer::~FrameBuffer()
{
    device.destroy(fbuffer);
}

void FrameBuffer::prepare(RenderPass& rpass, std::vector<ImageView>& imageViews, uint32_t w, uint32_t h, uint32_t layerCount)
{
	assert(imageViews.size() > 0);

	// store locally the screen extents for use later
	width = w;
	height = h;

	vk::FramebufferCreateInfo frameInfo({}, rpass.get(), imageViews.size(), imageViews.data(), width, height,
	                                    layerCount);

	VK_CHECK_RESULT(device.createFramebuffer(&frameInfo, nullptr, &framebuffer));
}


}    // namespace VulkanAPI
