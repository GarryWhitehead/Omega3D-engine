#include "RenderPass.h"

namespace VulkanAPI
{

	RenderPass::RenderPass(vk::Device dev) :
		device(dev)
	{
	}

	RenderPass::RenderPass(vk::Device dev, vk::RenderPass pass) :
		renderpass(pass),
		device(dev)
	{
		assert(renderpass != VK_NULL_HANDLE);
	}



	RenderPass::~RenderPass()
	{
		destroy();
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

	void RenderPass::addReference(const vk::ImageLayout layout, const uint32_t attachId)
	{
		vk::AttachmentReference ref = {};

		if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			ref.attachment = attachId;
			ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depthReference.push_back(ref);
		}
		else {
			ref.attachment = attachId;
			ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
			colorReference.push_back(ref);
		}
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
		dependency.push_back(depend);
	}

	void RenderPass::prepareRenderPass()
	{
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
		assert(imageView != VK_NULL_HANDLE);
		assert(renderpass != VK_NULL_HANDLE);

		vk::FramebufferCreateInfo frameInfo({},
		renderpass, 
		1, &imageView,
		width, height,
		layerCount);

		VK_CHECK_RESULT(device.createFramebuffer(&frameInfo, nullptr, &frameBuffer))
	}

	void RenderPass::prepareFramebuffer(std::vector<vk::ImageView>& imageView, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		assert(!imageView.empty());
		assert(renderpass != VK_NULL_HANDLE);

		vk::FramebufferCreateInfo frameInfo({},
		renderpass,
		static_cast<uint32_t>(imageView.size()),
		imageView.data(),
		width, height,
		layerCount);

		VK_CHECK_RESULT(device.createFramebuffer(&frameInfo, nullptr, &frameBuffer))
	}

	void RenderPass::destroy()
	{
		// its not always the case that the user might create a framebuffer using this class, so check
		if (frameBuffer != VK_NULL_HANDLE) {
			device.destroyFramebuffer(frameBuffer, nullptr);
		}
		device.destroyRenderPass(renderpass, nullptr);

		device = VK_NULL_HANDLE;
	}

}

