#include "DeferredRenderer.h"
#include "Utility/logger.h"
#include "Rendering/RenderInterface.h"
#include "Engine/Omega_Global.h"
#include "Managers/CameraManager.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Queue.h"
#include "Vulkan/SemaphoreManager.h"
#include "PostProcess/PostProcessInterface.h"
#include "Vulkan/Device.h"

namespace OmegaEngine
{
	
	DeferredRenderer::DeferredRenderer(vk::Device dev, vk::PhysicalDevice physical, RenderConfig _render_config) :
		device(dev),
		gpu(physical),
		render_config(_render_config)
	{
		
	}


	DeferredRenderer::~DeferredRenderer()
	{
	}

	void DeferredRenderer::create_gbuffer_pass()
	{
		// a list of the formats required for each buffer
		vk::Format depth_format = VulkanAPI::Device::get_depth_format(gpu);

		std::vector<VulkanAPI::RenderPass::AttachedFormat> attachments =
		{
			{ vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },		// position
			{ vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eColorAttachmentOptimal },			// Albedo
			{ vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },		// Normal
			{ vk::Format::eR32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },				// Pbr material
			{ vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },			// emissive
			{ depth_format, vk::ImageLayout::eDepthStencilAttachmentOptimal }					// depth

		};
		const uint8_t num_attachments = attachments.size();

		// create a empty texture for each state - these will be filled by the shader
		for (uint8_t i = 0; i < num_attachments - 1; ++i) {
			gbuffer_images[i].create_empty_image(device, gpu, attachments[i].format, render_config.deferred.gbuffer_width, render_config.deferred.gbuffer_height, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
		}

		// and the depth g-buffer
		gbuffer_images[num_attachments - 1].create_empty_image(device, gpu, depth_format, render_config.deferred.gbuffer_width, render_config.deferred.gbuffer_height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// now create the renderpasses and frame buffers
		gbuffer_renderpass.init(device, attachments);

		// tie the image-views to the frame buffer
		std::vector<vk::ImageView> image_views(num_attachments);

		for (uint8_t i = 0; i < num_attachments; ++i) {
			image_views[i] = gbuffer_images[i].get_image_view();
		}
		gbuffer_renderpass.prepareFramebuffer(static_cast<uint32_t>(image_views.size()), image_views.data(), render_config.deferred.gbuffer_width, render_config.deferred.gbuffer_height);
	}


	void DeferredRenderer::create_deferred_pass(uint32_t width, uint32_t height, CameraManager& camera_manager)
	{
		// if we are using the colour image for further manipulation (e.g. post-process) render into full screen buffer, otherwise render into swapchain buffer
		vk::Format deferred_format = vk::Format::eR32G32B32A32Sfloat;
		image.create_empty_image(device, gpu, deferred_format, render_config.deferred.offscreen_width, render_config.deferred.offscreen_height, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

		// now create the renderpasses and frame buffers
		renderpass.init(device);
		renderpass.addAttachment(vk::ImageLayout::eColorAttachmentOptimal, deferred_format);
		renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, 0);
		renderpass.prepareRenderPass();
		renderpass.prepareFramebuffer(image.get_image_view(), render_config.deferred.offscreen_width, render_config.deferred.offscreen_height, 1);

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
		
		// only one buffer. This should be imporved - somehow automate the association of shader buffers and their assoicated memory allocations
		descr_set.write_set(buffer_layout[0].set, buffer_layout[0].binding, buffer_layout[0].type, camera_manager.get_ubo_buffer(), camera_manager.get_ubo_offset(), buffer_layout[0].range);

		// and finally create the pipeline
		// first finish of the pipeline layout....
		pl_layout.create(device, descr_layout.get_layout());

		pipeline.set_depth_state(VK_TRUE, VK_FALSE);
		pipeline.add_dynamic_state(vk::DynamicState::eLineWidth);
		pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		pipeline.add_colour_attachment(VK_FALSE, renderpass);
		pipeline.set_raster_front_face(vk::FrontFace::eClockwise);
		pipeline.create(device, renderpass, shader, pl_layout, VulkanAPI::PipelineType::Graphics);
	}


	void DeferredRenderer::render_deferred(VulkanAPI::Queue& graph_queue, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore)
	{
		cmd_buffer.init(device);
		cmd_buffer.create_primary();

		// begin the renderpass 
		vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
		cmd_buffer.begin_renderpass(begin_info);

		// bind everything required to draw
		cmd_buffer.bind_pipeline(pipeline);
		cmd_buffer.bind_descriptors(pl_layout, descr_set, VulkanAPI::PipelineType::Graphics);

		// render full screen quad to screen
		cmd_buffer.draw_indexed_quad();

		// submit to graphics queue
		graph_queue.submit_cmd_buffer(cmd_buffer.get(), wait_semaphore, signal_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}


	void DeferredRenderer::render(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface)
	{
		// Note: only deferred supported at the moment but this will change once Forward rendering is added
		// first stage of the deferred render pipeline is to generate the g-buffers by drawing the components into the offscreen frame-buffers
		// This is done by the render interface. A little back and forth, but these functions are used by all renderers
		render_interface->render_components();

		// Now for the deferred specific rendering pipeline - render the deffered pass - lights and IBL
		vk::Semaphore deferred_semaphore = VulkanAPI::Global::Managers::semaphore_manager.get_semaphore();
		render_deferred(vk_interface->get_graph_queue(), vk_interface->get_swapchain_semaphore(), deferred_semaphore);

		// post-processing is done in a separate forward pass using the offscreen buffer filled by the deferred pass
		if (render_config.general.use_post_process) {

			vk::Semaphore post_semaphore = VulkanAPI::Global::Managers::semaphore_manager.get_semaphore();
			pp_interface->render();
		}
		else {
			// if post-processing isn't wanted, render the final composition as a full-screen quad

		}
	}

	std::function<void()> DeferredRenderer::set_render_callback(RenderInterface* render_interface, std::unique_ptr<VulkanAPI::Interface>& vk_interface)
	{
		auto& func = [&]() { render(render_interface, vk_interface); };
		return func;
	}
}
