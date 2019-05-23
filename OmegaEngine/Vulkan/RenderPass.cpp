#include "RenderPass.h"
#include "Vulkan/Device.h"

namespace VulkanAPI
{

	RenderPass::RenderPass()
	{

	}

	RenderPass::RenderPass(vk::Device dev) :
		device(dev)
	{
	}

	RenderPass::RenderPass(vk::Device dev, vk::RenderPass pass) :
		renderpass(pass),
		device(dev)
	{
		assert(renderpass);
	}

	RenderPass::~RenderPass()
	{
	}

	void RenderPass::init(vk::Device dev)
	{
		device = dev;
	}

	void RenderPass::addAttachment(const vk::ImageLayout finalLayout, const vk::Format format)
	{
		vk::AttachmentDescription attachDescr({},
			format,
			vk::SampleCountFlagBits::e1,	
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined, finalLayout);

		attachment.push_back(attachDescr);
	}

	void RenderPass::addSubPass(std::vector<vk::AttachmentReference>& colorRef, std::vector<vk::AttachmentReference>& inputRef, vk::AttachmentReference *depthRef)
	{
		assert(!colorRef.empty());
		assert(!inputRef.empty());

		// override default subpass with user specified subpass
		vk::SubpassDescription subpassDescr = {};
		subpassDescr.pipelineBindPoint = vk::PipelineBindPoint::eGraphics; 

		// colour attachements
		subpassDescr.colorAttachmentCount = static_cast<uint32_t>(colorRef.size());
		subpassDescr.pColorAttachments = colorRef.data();

		// input attachments
		subpassDescr.inputAttachmentCount = static_cast<uint32_t>(inputRef.size());
		subpassDescr.pInputAttachments = inputRef.data();

		// depth attachment - if required
		if (depthRef != nullptr) {

			subpassDescr.pDepthStencilAttachment = depthRef;
		}

		subpass.push_back(subpassDescr);
	}

	void RenderPass::addSubPass(std::vector<vk::AttachmentReference>& colorRef, vk::AttachmentReference *depthRef)
	{
		assert(!colorRef.empty());

		// override default subpass with user specified subpass
		vk::SubpassDescription subpassDescr = {};
		subpassDescr.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

		// colour attachements
		subpassDescr.colorAttachmentCount = static_cast<uint32_t>(colorRef.size());
		subpassDescr.pColorAttachments = colorRef.data();

		// depth attachment - if required
		if (depthRef != nullptr) {

			subpassDescr.pDepthStencilAttachment = depthRef;
		}

		subpass.push_back(subpassDescr);
	}

	void RenderPass::addSubpassDependency(DependencyTemplate depend_template, uint32_t srcSubpass, uint32_t dstSubpass)
	{
		vk::SubpassDependency depend;

		if (depend_template == DependencyTemplate::Top_Of_Pipe) {

			depend.srcSubpass = VK_SUBPASS_EXTERNAL;
			depend.dstSubpass = 0;
			depend.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			depend.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			depend.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
			depend.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
			depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
		else if (depend_template == DependencyTemplate::Bottom_Of_Pipe) {

			depend.srcSubpass = 0;
			depend.dstSubpass = VK_SUBPASS_EXTERNAL;
			depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			depend.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			depend.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
			depend.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
			depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
		else if (depend_template == DependencyTemplate::Multi_Subpass) {

			depend.srcSubpass = srcSubpass;
			depend.dstSubpass = dstSubpass;
			depend.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			depend.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
			depend.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
		else if (depend_template == DependencyTemplate::Stencil_Subpass_Bottom) {

			depend.srcSubpass = VK_SUBPASS_EXTERNAL;
			depend.dstSubpass = 0;
			depend.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
			depend.dstStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
			depend.srcAccessMask = vk::AccessFlagBits::eShaderRead;
			depend.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
		else if (depend_template == DependencyTemplate::Stencil_Subpass_Fragment) {

			depend.srcSubpass = 0;
			depend.dstSubpass = VK_SUBPASS_EXTERNAL;
			depend.srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
			depend.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
			depend.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			depend.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
		else if (depend_template == DependencyTemplate::DepthStencilSubpassTop)
		{
			depend.srcSubpass = VK_SUBPASS_EXTERNAL;
			depend.dstSubpass = 0;
			depend.srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
			depend.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
			depend.srcAccessMask = vk::AccessFlagBits::eShaderRead;
			depend.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			depend.dependencyFlags = vk::DependencyFlagBits::eByRegion;
		}
		else if (depend_template == DependencyTemplate::DepthStencilSubpassBottom)
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
		assert(device);
		assert(!attachment.empty());

		// create the colour .depth refs
		uint32_t attach_id = 0;
		for (auto& attach : attachment) {

			if (attach.finalLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
				depthReference.push_back({ attach_id, vk::ImageLayout::eDepthStencilAttachmentOptimal });
			}
			else {
				colorReference.push_back({ attach_id, vk::ImageLayout::eColorAttachmentOptimal });
			}
			++attach_id;
		}

		// if dependency container is empty, go with the default layout
		if (dependency.empty()) {

			vk::SubpassDependency depend_top(VK_SUBPASS_EXTERNAL, 0,
				vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::AccessFlagBits::eMemoryRead,
				vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
				vk::DependencyFlagBits::eByRegion);
			dependency.push_back(depend_top);

			vk::SubpassDependency depend_bott(0, VK_SUBPASS_EXTERNAL,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::DependencyFlagBits::eByRegion);
			dependency.push_back(depend_bott);
		}

		// if subpass vector empty, use default subpass layout
		if (subpass.empty()) {

			vk::SubpassDescription sPassDescr;
			sPassDescr.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

			if (!colorReference.empty()) {
				sPassDescr.colorAttachmentCount = static_cast<uint32_t>(colorReference.size());
				sPassDescr.pColorAttachments = colorReference.data();
			}
			if (!depthReference.empty()) {
				sPassDescr.pDepthStencilAttachment = depthReference.data();
			}
			subpass.push_back(sPassDescr);
		}

		vk::RenderPassCreateInfo createInfo({},
		static_cast<uint32_t>(attachment.size()),
		attachment.data(),
		static_cast<uint32_t>(subpass.size()),
		subpass.data(),
		static_cast<uint32_t>(dependency.size()),
		dependency.data());

		VK_CHECK_RESULT(device.createRenderPass(&createInfo, nullptr, &renderpass));
	}

	void RenderPass::prepareFramebuffer(const vk::ImageView imageView, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		assert(imageView);
		assert(renderpass);

		// store locally the screen extents for use later
		image_width = width;
		image_height = height;

		vk::FramebufferCreateInfo frameInfo({},
		renderpass, 
		1, &imageView,
		width, height,
		layerCount);

		vk::Framebuffer framebuffer;
		VK_CHECK_RESULT(device.createFramebuffer(&frameInfo, nullptr, &framebuffer));
		framebuffers.push_back(framebuffer);
	}

	void RenderPass::prepareFramebuffer(uint32_t size, vk::ImageView* imageView, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		assert(imageView);
		assert(renderpass);

		// store locally the screen extents for use later
		image_width = width;
		image_height = height;

		vk::FramebufferCreateInfo frameInfo({},
		renderpass,
		size, imageView,
		width, height,
		layerCount);

		vk::Framebuffer framebuffer;
		VK_CHECK_RESULT(device.createFramebuffer(&frameInfo, nullptr, &framebuffer));
		framebuffers.push_back(framebuffer);
	}

	vk::RenderPassBeginInfo RenderPass::getBeginInfo(vk::ClearColorValue& bg_colour, uint32_t fb_index)
	{
		// set up clear colour for each colour attachment
		clear_values.resize(attachment.size());
		for (uint32_t i = 0; i < attachment.size(); ++i) {
			if (attachment[i].finalLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
				clear_values[i].depthStencil = { 1.0f, 0 }; 
			}
			else {
				clear_values[i].color = bg_colour;
			}
		}
		
		vk::RenderPassBeginInfo beginInfo(
			renderpass, framebuffers[fb_index],
			{ { 0, 0 }, { image_width, image_height } },
			static_cast<uint32_t>(clear_values.size()),
			clear_values.data());

		return beginInfo;
	}

	vk::RenderPassBeginInfo RenderPass::getBeginInfo(uint32_t size, vk::ClearValue* colour, uint32_t fb_index)
	{

		vk::RenderPassBeginInfo beginInfo(
			renderpass, framebuffers[fb_index],
			{ { 0, 0 }, { image_width, image_height } },
			size,
			colour);

		return beginInfo;
	}

}

