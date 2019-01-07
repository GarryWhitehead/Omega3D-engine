#pragma once
#include "Vulkan/Common.h"
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
		Count
	};

	class RenderPass
	{

	public:
		RenderPass(vk::Device dev);
		RenderPass(vk::Device dev, vk::RenderPass pass);
		~RenderPass();

		void addAttachment(const vk::ImageLayout finalLayout, const vk::Format format);
		void addSubPass(std::vector<vk::AttachmentReference>& colorRef, std::vector<vk::AttachmentReference>& inputRef, vk::AttachmentReference *depthRef = nullptr);
		void addSubPass(std::vector<vk::AttachmentReference>& colorRef, vk::AttachmentReference *depthRef = nullptr);																		// override without input attachments
		void addSubpassDependency(DependencyTemplate depend_template, uint32_t srcSubpass = 0, uint32_t dstSubpass = 0);											// templated version
		void addReference(const vk::ImageLayout layout, const uint32_t attachId);
		void prepareRenderPass();
		void prepareFramebuffer(const vk::ImageView imageView, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void prepareFramebuffer(uint32_t size, vk::ImageView* imageView, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void destroy();



	private:

		vk::Device device;

		vk::RenderPass renderpass;
		vk::Framebuffer frameBuffer;
		std::vector<vk::AttachmentDescription> attachment;
		std::vector<vk::AttachmentReference> colorReference;
		std::vector<vk::AttachmentReference> depthReference;
		std::vector<vk::SubpassDescription> subpass;
		std::vector<vk::SubpassDependency> dependency;


	};

}

