#include "DeferredRenderer.h"
#include "Utility/logger.h"
#include "Rendering/RenderInterface.h"
#include "Engine/Omega_Global.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Queue.h"
#include "Vulkan/SemaphoreManager.h"
#include "PostProcess/PostProcessInterface.h"

namespace OmegaEngine
{
	
	DeferredRenderer::DeferredRenderer(vk::Device dev) :
		device(dev)
	{
		
	}


	DeferredRenderer::~DeferredRenderer()
	{
	}

	void DeferredRenderer::create(uint32_t width, uint32_t height)
	{
		// a list of the formats required for each buffer
		vk::Format depth_format = VulkanAPI::Util::get_depth_format();

		std::vector<VulkanAPI::RenderPass::AttachedFormat> attachments = 
		{
			{ vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },		// position
			{ vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eColorAttachmentOptimal },			// Albedo
			{ vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },		// Normal
			{ vk::Format::eR32Sfloat, vk::ImageLayout::eColorAttachmentOptimal },				// Pbr material
			{ depth_format, vk::ImageLayout::eDepthStencilAttachmentOptimal }					// depth

		};

		// create a empty texture for each state - these will be filled by the shader
		for (uint8_t i = 0; i < attachments.size() - 1; ++i) {
			images[i].create_empty_image(attachments[i].format, width, height, 1, vk::ImageUsageFlagBits::eColorAttachment);
		}

		// and the depth g-buffer
		images[(int)DeferredAttachments::Depth].create_empty_image(depth_format, width, height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// now create the renderpasses and frame buffers
		std::vector<VulkanAPI::DependencyTemplate> dependencies{ VulkanAPI::DependencyTemplate::Top_Of_Pipe, VulkanAPI::DependencyTemplate::Bottom_Of_Pipe };
		renderpass = std::make_unique<VulkanAPI::RenderPass>(attachments, dependencies);

		// tie the image-views to the frame buffer
		std::array<vk::ImageView, (int)DeferredAttachments::Count> image_views;

		for (uint8_t i = 0; i < attachments.size(); ++i) {
			image_views[i] = images[i].get_image_view();
		}
		renderpass->prepareFramebuffer(image_views.size(), image_views.data(), Global::program_state.get_win_width(), Global::program_state.get_win_height());

		// load the shaders and carry out reflection to create the pipeline and descriptor layouts
		shader.add(device, "renderer/deferred.vert", VulkanAPI::StageType::Vertex, "renderer/deferred.frag", VulkanAPI::StageType::Fragment);

		std::vector<VulkanAPI::ShaderImageLayout> sampler_layout;
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		shader.reflection(VulkanAPI::StageType::Vertex, *descr_layout, pl_layout, *pipeline, sampler_layout, buffer_layout);

		assert(!sampler_layout.empty());
		assert(!buffer_layout.empty());

		// descriptor sets for all the deferred buffers samplers
		// using a linear sampler for all deferred buffers
		linear_sampler = std::make_unique<VulkanAPI::Sampler>(VulkanAPI::SamplerType::LinearClamp);
		for (uint8_t i = 0; i < sampler_layout.size(); ++i) {
			descr_set->update_set(sampler_layout[i].binding, vk::DescriptorType::eSampler, linear_sampler->get_sampler(), image_views[i], attachments[i].layout);
		}
		for (uint8_t i = 0; i < buffer_layout.size(); ++i) {
			descr_set->update_set(i, vk::DescriptorType::eSampler, image_views[i], attachments[i].layout);
		}
		
		// and finally create the pipeline
		pipeline->set_depth_state(VK_TRUE, VK_FALSE);
		pipeline->add_dynamic_state(vk::DynamicState::eLineWidth);
		pipeline->set_topology(vk::PrimitiveTopology::eTriangleList);
		pipeline->add_colour_attachment(VK_FALSE);
		pipeline->set_raster_front_face(vk::FrontFace::eClockwise);
		pipeline->create();
	}

	void DeferredRenderer::render_deferred(VulkanAPI::Queue& graph_queue, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore)
	{
		cmd_buffer.init(device);
		cmd_buffer.create_primary();

		// begin the renderpass 
		vk::RenderPassBeginInfo begin_info = renderpass->get_begin_info(render_config.general.background_col);
		cmd_buffer.begin_renderpass(begin_info);

		// bind everything required to draw
		cmd_buffer.bind_pipeline(*pipeline);
		cmd_buffer.bind_descriptors(pl_layout, *descr_set, VulkanAPI::PipelineType::Graphics);

		// set push constants for light variables

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
		render_deferred(vk_interface->get_graph_queue, vk_interface->get_swapchain_semaphore, deferred_semaphore);

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
