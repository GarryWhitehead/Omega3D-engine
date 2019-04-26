#include "DeferredRenderer.h"
#include "Utility/logger.h"
#include "Rendering/RenderInterface.h"
#include "Engine/Omega_Global.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/ComponentInterface.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Queue.h"
#include "Vulkan/SemaphoreManager.h"
#include "PostProcess/PostProcessInterface.h"
#include "Vulkan/Device.h"

namespace OmegaEngine
{
	
	DeferredRenderer::DeferredRenderer(vk::Device& dev, vk::PhysicalDevice& physical, RenderConfig _render_config) :
		device(dev),
		gpu(physical),
		render_config(_render_config),
		RendererBase(RendererType::Deferred)
	{
	}


	DeferredRenderer::~DeferredRenderer()
	{
	}

	void DeferredRenderer::create_gbuffer_pass()
	{
		// a list of the formats required for each buffer
		vk::Format depth_format = VulkanAPI::Device::get_depth_format(gpu);

		first_renderpass.init(device);
		first_renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16B16A16Sfloat);		// position
		first_renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR8G8B8A8Unorm);			// colour
		first_renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16B16A16Sfloat);		// normal
		first_renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16Sfloat);				// pbr
		first_renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16B16A16Sfloat);		// emissive
		first_renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depth_format);					// depth
		first_renderpass.prepareRenderPass();

		const uint8_t num_attachments = 6;

		// create a empty texture for each state - these will be filled by the shader
		for (uint8_t i = 0; i < num_attachments - 1; ++i) {
			gbuffer_images[i].create_empty_image(device, gpu, first_renderpass.get_attachment_format(i), render_config.deferred.gbuffer_width, render_config.deferred.gbuffer_height, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
		}

		// and the depth g-buffer
		gbuffer_images[num_attachments - 1].create_empty_image(device, gpu, depth_format, render_config.deferred.gbuffer_width, render_config.deferred.gbuffer_height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// tie the image-views to the frame buffer
		std::vector<vk::ImageView> image_views(num_attachments);

		for (uint8_t i = 0; i < num_attachments; ++i) {
			image_views[i] = gbuffer_images[i].get_image_view();
		}
		first_renderpass.prepareFramebuffer(static_cast<uint32_t>(image_views.size()), image_views.data(), render_config.deferred.gbuffer_width, render_config.deferred.gbuffer_height);
	}


	void DeferredRenderer::create_deferred_pass(std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager, RenderInterface* render_interface)
	{
		// if we are using the colour image for further manipulation (e.g. post-process) render into full screen buffer, otherwise render into swapchain buffer
		if (render_config.general.use_post_process) {

			vk::Format deferred_format = vk::Format::eR32G32B32A32Sfloat;
			
			// now create the renderpasses and frame buffers
			renderpass.init(device);
			renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, deferred_format);
			renderpass.prepareRenderPass();

			image.create_empty_image(device, gpu, deferred_format, render_config.deferred.offscreen_width, render_config.deferred.offscreen_height, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
			renderpass.prepareFramebuffer(image.get_image_view(), render_config.deferred.offscreen_width, render_config.deferred.offscreen_height, 1);
		}
		
		// load the shaders and carry out reflection to create the pipeline and descriptor layouts
		if (!shader.add(device, "renderer/deferred/deferred-vert.spv", VulkanAPI::StageType::Vertex, "renderer/deferred/deferred-frag.spv", VulkanAPI::StageType::Fragment)) {
			LOGGER_ERROR("Unable to load deferred renderer shaders.");
			throw std::runtime_error("Error whilst trying to open deferred shader file.");
		}

		// create the descriptors and pipeline layout through shader reflection
		VulkanAPI::ImageLayoutBuffer sampler_layout;
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		shader.descriptor_buffer_reflect (descr_layout, buffer_layout);
		shader.descriptor_image_reflect(descr_layout, sampler_layout);
		shader.pipeline_layout_reflect(pl_layout);
		shader.pipeline_reflection(pipeline);

		descr_layout.create(device);
		descr_set.init(device, descr_layout);

		// not completely automated! We still need to manually adjust the set numbers for each type
		const uint8_t DeferredSet = 1;
		const uint8_t EnvironmentSet = 2;

		for (uint8_t i = 0; i < sampler_layout[DeferredSet].size(); ++i) {
			descr_set.write_set(sampler_layout[DeferredSet][i], gbuffer_images[i].get_image_view());
		}
		
		for (auto& layout : buffer_layout) {

			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				buffer_manager->enqueueDescrUpdate("Camera", &descr_set, layout.set, layout.binding, layout.type);
			}
			if (layout.name == "LightUbo") {
				buffer_manager->enqueueDescrUpdate("Light", &descr_set, layout.set, layout.binding, layout.type);
			}
		}

		// and finally create the pipeline
		// first finish of the pipeline layout....
		pl_layout.create(device, descr_layout.get_layout());

		pipeline.set_depth_state(VK_TRUE, VK_TRUE);
		pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		pipeline.set_raster_front_face(vk::FrontFace::eClockwise);
		pipeline.set_raster_cull_mode(vk::CullModeFlagBits::eBack);
		
		if (render_config.general.use_post_process) {
			pipeline.add_colour_attachment(VK_FALSE, renderpass);
			pipeline.create(device, renderpass, shader, pl_layout, VulkanAPI::PipelineType::Graphics);
		}
		else {
			// render to the swapchain presentation 
			pipeline.add_colour_attachment(VK_FALSE, render_interface->get_swapchain_renderpass());
			pipeline.create(device, render_interface->get_swapchain_renderpass(), shader, pl_layout, VulkanAPI::PipelineType::Graphics);
		}
	}


	void DeferredRenderer::render_deferred(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::CommandBufferManager>& cmd_buffer_manager)
	{
		// the renderpass depends wheter we are going to forward render the deferred pass into a offscreen buffer for transparency, sampling, etc.
		// or just render straight to the swap chain presentation image
		if (render_config.general.use_post_process) {
			
			auto& cmd_buffer = cmd_buffer_manager->get_cmd_buffer(cmd_buffer_handle);
			cmd_buffer->create_primary();

			// begin the renderpass 
			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
			cmd_buffer->begin_renderpass(begin_info);

			// viewport and scissor
			cmd_buffer->set_viewport();
			cmd_buffer->set_scissor();

			// bind everything required to draw
			cmd_buffer->bind_pipeline(pipeline);
			cmd_buffer->bind_descriptors(pl_layout, descr_set, VulkanAPI::PipelineType::Graphics);
			cmd_buffer->bind_push_block(pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(RenderConfig::IBLInfo), &render_config.ibl);

			// render full screen quad to screen
			cmd_buffer->draw_quad();

			// end this pass and cmd buffer
			cmd_buffer->end_pass();
			cmd_buffer->end();
		}
		else {
			for (uint32_t i = 0; i < render_interface->get_swapchain_count(); ++i) {

				auto& cmd_buffer = cmd_buffer_manager->get_present_cmd_buffer();

				// bind everything required to draw
				cmd_buffer->bind_pipeline(pipeline);
				cmd_buffer->bind_descriptors(pl_layout, descr_set, VulkanAPI::PipelineType::Graphics);
				cmd_buffer->bind_push_block(pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(RenderConfig::IBLInfo), &render_config.ibl);

				// render full screen quad to screen
				cmd_buffer->draw_quad();
				cmd_buffer->end_pass();
				cmd_buffer->end();
			}
		}
	}


	void DeferredRenderer::render(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface)
	{
		auto& cmd_buffer_manager = vk_interface->get_cmd_buffer_manager();

		// Note: only deferred supported at the moment but this will change once Forward rendering is added
		// first stage of the deferred render pipeline is to generate the g-buffers by drawing the components into the offscreen frame-buffers
		// This is done by the render interface. A little back and forth, but these functions are used by all renderers
		render_interface->render_components(render_config, first_renderpass);

		// Now for the deferred specific rendering pipeline - render the deffered pass - lights and IBL
		render_deferred(render_interface, cmd_buffer_manager);

		// post-processing is done in a separate forward pass using the offscreen buffer filled by the deferred pass
		if (render_config.general.use_post_process) {
			
			pp_interface->render();
		}
		
		// finally send to the swap-chain presentation
		cmd_buffer_manager->submit_frame(vk_interface->get_swapchain());
	}
}
