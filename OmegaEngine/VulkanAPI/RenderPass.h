#pragma once
#include "VulkanAPI/Common.h"
#include <vector>
#include <assert.h>

namespace VulkanAPI
{


	enum class DependencyTemplate
	{
		Top_Of_Pipe,
		Bottom_Of_Pipe,
		Multi_Subpass,
		Stencil_Subpass_Bottom,
		Stencil_Subpass_Fragment,
		DepthStencilSubpassTop,
		DepthStencilSubpassBottom,
	};

	class RenderPass
	{

	public:

		struct AttachedFormat
		{
			vk::Format format;
			vk::ImageLayout layout;
		};

		RenderPass();
		RenderPass(vk::Device dev);
		RenderPass(vk::Device dev, vk::RenderPass pass);

		~RenderPass();

		vk::RenderPass get()
		{
			return renderpass;
		}

		uint32_t get_attach_count() const
		{
			return static_cast<uint32_t>(colorReference.size());
		}

		uint32_t getImageWidth() const
		{
			return imageWidth;
		}

		uint32_t getImageHeight() const
		{
			return imageHeight;
		}

		vk::Format& get_attachment_format(uint32_t index)
		{
			assert(!attachment.empty() && index < attachment.size());
			return attachment[index].format;
		}

		void init(vk::Device dev);

		void addAttachment(const vk::ImageLayout finalLayout, const vk::Format format, bool clearAttachment = true);
		void addSubPass(std::vector<vk::AttachmentReference>& colorRef, std::vector<vk::AttachmentReference>& inputRef, vk::AttachmentReference *depthRef = nullptr);
		void addSubPass(std::vector<vk::AttachmentReference>& colorRef, vk::AttachmentReference *depthRef = nullptr);												// override without input attachments
		void addSubpassDependency(DependencyTemplate dependencyTemplate, uint32_t srcSubpass = 0, uint32_t dstSubpass = 0);											// templated version
		void prepareRenderPass();

		// frame buffer functions
		void prepareFramebuffer(const vk::ImageView imageView, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void prepareFramebuffer(uint32_t size, vk::ImageView* imageView, uint32_t width, uint32_t height, uint32_t layerCount = 1);

		// for generating cmd buffer
		vk::RenderPassBeginInfo getBeginInfo(vk::ClearColorValue& backgroundColour, uint32_t index = 0);
		vk::RenderPassBeginInfo getBeginInfo(uint32_t size, vk::ClearValue* backgroundColour, uint32_t index = 0);
		vk::RenderPassBeginInfo getBeginInfo(uint32_t index = 0);

	private:

		vk::Device device;

		vk::RenderPass renderpass;
		std::vector<vk::Framebuffer> framebuffers;

		uint32_t imageWidth = 0; 
		uint32_t imageHeight = 0;

		std::vector<vk::AttachmentDescription> attachment;
		std::vector<vk::AttachmentReference> colorReference;
		std::vector<vk::AttachmentReference> depthReference;
		std::vector<vk::SubpassDescription> subpass;
		std::vector<vk::SubpassDependency> dependency;

		// local store of clear values for render passes
		std::vector<vk::ClearValue> clearValues;
	};

}

