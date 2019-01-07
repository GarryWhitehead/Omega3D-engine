#include "Renderer.h"
#include "Utility/logger.h"
#include "Engine/Omega_Global.h"

namespace OmegaEngine
{

	Renderer::Renderer(RendererType r_type) :
		type(r_type)
	{

		// let's do all the initilisations required ready for importing objects for rendering
		// load in the renderer_config file
		
		// setup the renderer pipeline
		switch (r_type) {
		case RendererType::Deferred:
			setup_deferred_renderer();
			break;
		default:
			LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
			break;
		}
	}


	Renderer::~Renderer()
	{
	}

	void Renderer::setup_deferred_renderer(uint32_t width, uint32_t height)
	{
		struct AttachedFormat
		{
			DeferredAttachments type;
			vk::Format format;
		};

		// a list of the formats required for each buffer
		std::vector<AttachedFormat> attached = 
		{
			{ DeferredAttachments::Position, vk::Format::eR16G16B16A16Sfloat },
			{ DeferredAttachments::Albedo, vk::Format::eR8G8B8A8Unorm },
			{ DeferredAttachments::Normal,  vk::Format::eR16G16B16A16Sfloat },
			{ DeferredAttachments::Bump, vk::Format::eR8G8B8A8Unorm },
			{ DeferredAttachments::Occlusion, vk::Format::eR16Sfloat },
			{ DeferredAttachments::Metallic, vk::Format::eR16Sfloat },
			{ DeferredAttachments::Roughness, vk::Format::eR16Sfloat },
		};

		for (auto& attach : attached) {
			renderer_info.images[(int)attach.type].create_empty_image(attach.format, width, height, 1, vk::ImageUsageFlagBits::eColorAttachment);
		}

		// and the g-buffer
		vk::Format depth_format = VulkanAPI::Util::get_depth_format();
		renderer_info.images[(int)DeferredAttachments::Depth].create_empty_image(depth_format, width, height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// now create the renderpasses and frame buffers
		// attachments and subpass colour references
		for (auto& attach : attached) {
			renderer_info.renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, attach.format);
			renderer_info.renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, static_cast<uint32_t>(attach.type));
		}
		renderer_info.renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depth_format);
		renderer_info.renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, static_cast<uint32_t>(DeferredAttachments::Depth));

		renderer_info.renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::Top_Of_Pipe);
		renderer_info.renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::Bottom_Of_Pipe);
		renderer_info.renderpass.prepareRenderPass();

		// tie the image-views to the frame buffer
		std::array<vk::ImageView, (int)DeferredAttachments::Count> image_views;
		for (auto& attach : attached) {
			int index = static_cast<int>(attach.type);
			image_views[index] = renderer_info.images[index].get_image_view();
		}
		renderer_info.renderpass.prepareFramebuffer(image_views.size(), image_views.data(), Global::programState.width, Global::programState.height);

		// load the shaders and carry out reflection to create the pipeline and descriptor layouts
		renderer_info.shader.add(device, "renderer/deferred.vert", VulkanAPI::StageType::Vertex, "renderer/deferred.frag", VulkanAPI::StageType::Fragment);

		// finally create the deferred pipeline
	}


}
