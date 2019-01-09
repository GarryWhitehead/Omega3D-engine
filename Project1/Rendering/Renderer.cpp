#include "Renderer.h"
#include "Utility/logger.h"
#include "Engine/Omega_Global.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/BufferManager.h"

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
			def_renderer = std::make_unique<DeferredInfo>();
			def_renderer->create(device, width, height);
			break;
		default:
			LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
			break;
		}
	}


	Renderer::~Renderer()
	{
	}

	void Renderer::DeferredInfo::create(vk::Device device, uint32_t width, uint32_t height)
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
			images[(int)attach.type].create_empty_image(attach.format, width, height, 1, vk::ImageUsageFlagBits::eColorAttachment);
		}

		// and the g-buffer
		vk::Format depth_format = VulkanAPI::Util::get_depth_format();
		images[(int)DeferredAttachments::Depth].create_empty_image(depth_format, width, height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// now create the renderpasses and frame buffers
		// attachments and subpass colour references
		for (auto& attach : attached) {
			renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, attach.format);
			renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, static_cast<uint32_t>(attach.type));
		}
		renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depth_format);
		renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, static_cast<uint32_t>(DeferredAttachments::Depth));

		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::Top_Of_Pipe);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::Bottom_Of_Pipe);
		renderpass.prepareRenderPass();

		// tie the image-views to the frame buffer
		std::array<vk::ImageView, (int)DeferredAttachments::Count> image_views;
		for (auto& attach : attached) {
			int index = static_cast<int>(attach.type);
			image_views[index] = images[index].get_image_view();
		}
		renderpass.prepareFramebuffer(image_views.size(), image_views.data(), Global::programState.width, Global::programState.height);

		// load the shaders and carry out reflection to create the pipeline and descriptor layouts
		shader.add(device, "renderer/deferred.vert", VulkanAPI::StageType::Vertex, "renderer/deferred.frag", VulkanAPI::StageType::Fragment);
		shader.reflection(VulkanAPI::StageType::Vertex, descr_layout, pl_layout);

		// setup buffers required by the shaders
		vert_buffer = VulkanAPI::Global::vk_managers.buff_manager->create(sizeof(VertBuffer));
		frag_buffer = VulkanAPI::Global::vk_managers.buff_manager->create(sizeof(FragBuffer));

		// descriptor sets for all the deferred buffers samplers
		for (uint8_t bind = 0; bind < (uint8_t)DeferredAttachments::Count; ++bind) {
			
			descr_set.update_set(bind, vk::DescriptorType::eSampler, Sampler::get_default_sampler(), image_views[bind]);
		}

	}


}
