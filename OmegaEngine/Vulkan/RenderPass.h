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

		struct AttachedFormat
		{
			vk::Format format;
			vk::ImageLayout layout;
		};

		RenderPass();
		RenderPass(vk::Device dev);
		RenderPass(vk::Device dev, vk::RenderPass pass);

		// create a pass from a list of info in one shot
		RenderPass(vk::Device dev, std::vector<AttachedFormat>& attach, std::vector<DependencyTemplate> dependencies);

		~RenderPass();

		vk::RenderPass get()
		{
			return renderpass;
		}

		uint32_t get_attach_count() const
		{
			return static_cast<uint32_t>(colorReference.size());
		}

		uint32_t get_image_width() const
		{
			return image_width;
		}

		uint32_t get_image_height() const
		{
			return image_height;
		}

		void init(vk::Device dev);
		void init(vk::Device dev, std::vector<AttachedFormat>& attach, std::vector<DependencyTemplate> dependencies);

		void addAttachment(const vk::ImageLayout finalLayout, const vk::Format format);
		void addSubPass(std::vector<vk::AttachmentReference>& colorRef, std::vector<vk::AttachmentReference>& inputRef, vk::AttachmentReference *depthRef = nullptr);
		void addSubPass(std::vector<vk::AttachmentReference>& colorRef, vk::AttachmentReference *depthRef = nullptr);												// override without input attachments
		void addSubpassDependency(DependencyTemplate depend_template, uint32_t srcSubpass = 0, uint32_t dstSubpass = 0);											// templated version
		void addReference(const vk::ImageLayout layout, const uint32_t attachId);
		void prepareRenderPass();

		// frame buffer functions
		void prepareFramebuffer(const vk::ImageView imageView, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void prepareFramebuffer(uint32_t size, vk::ImageView* imageView, uint32_t width, uint32_t height, uint32_t layerCount = 1);
		void destroy();

		// for generating cmd buffer
		vk::RenderPassBeginInfo get_begin_info(vk::ClearColorValue& bg_colour);

	private:

		vk::Device device;

		vk::RenderPass renderpass;
		vk::Framebuffer framebuffer;

		uint32_t image_width = 0; 
		uint32_t image_height = 0;

		std::vector<vk::AttachmentDescription> attachment;
		std::vector<vk::AttachmentReference> colorReference;
		std::vector<vk::AttachmentReference> depthReference;
		std::vector<vk::SubpassDescription> subpass;
		std::vector<vk::SubpassDependency> dependency;


	};

}

