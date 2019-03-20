#include "RenderInterface.h"
#include "RenderableTypes/RenderableBase.h"
#include "RenderableTypes/Mesh.h"
#include "Rendering/Renderers/DeferredRenderer.h"
#include "Managers/ComponentInterface.h"
#include "PostProcess/PostProcessInterface.h"
#include "Objects/Object.h"
#include "Objects/ObjectManager.h"
#include "Managers/TransformManager.h"
#include "Managers/CameraManager.h"
#include "Managers/MeshManager.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Vulkan/CommandBuffer.h"
#include "Utility/logger.h"
#include "Utility/FileUtil.h"
#include "Threading/ThreadPool.h"
#include "Engine/Omega_Global.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "Rendering/RenderQueue.h"

namespace OmegaEngine
{
	RenderInterface::RenderInterface()
	{
	}

	RenderInterface::RenderInterface(VulkanAPI::Device& device, std::unique_ptr<ComponentInterface>& component_interface)
	{
		init(device);
	}


	RenderInterface::~RenderInterface()
	{
	}

	void RenderInterface::init(VulkanAPI::Device& device)
	{
		// load the render config file if it exsists
		load_render_config();

		uint32_t win_width = Global::program_state.get_win_width();
		uint32_t win_height = Global::program_state.get_win_height();

		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, win_width, win_height);
		
		// all renderable elements will be dispatched for drawing via this queue
		render_queue = std::make_unique<RenderQueue>();

		// init the command buffer now ready for rendering later
		cmd_buffer.init(device.getDevice());

		// and also the swap chain presentation pass which will render the final composition to the screen
		prepare_swapchain_pass();
	}

	void RenderInterface::load_render_config()
	{
		std::string json;
		const char filename[] = "render_config.ini";		// probably need to check the current dir here
		if (!FileUtil::readFileIntoBuffer(filename, json)) {
			return;
		}

		// if we cant parse the config, then go with the default values
		rapidjson::Document doc;
		if (doc.Parse(json.c_str()).HasParseError()) {
			LOGGER_INFO("Unable to find render_config file. Using default settings...");
			return;
		}

		// general render settings
		if (doc.HasMember("Renderer")) {
			int renderer = doc["Renderer"].GetInt();
			render_config.general.renderer = static_cast<RendererType>(renderer);
		}
		
	}

	void RenderInterface::init_renderer(std::unique_ptr<ComponentInterface>& component_interface)
	{
		uint32_t win_width = Global::program_state.get_win_width();
		uint32_t win_height = Global::program_state.get_win_height();

		// setup the renderer pipeline
		switch (static_cast<RendererType>(render_config.general.renderer)) {
		case RendererType::Deferred:
		{
			set_renderer<DeferredRenderer>(vk_interface->get_device(), vk_interface->get_gpu(), render_config);
			auto deferred_renderer = reinterpret_cast<DeferredRenderer*>(renderer.get());
			deferred_renderer->create_gbuffer_pass();
			deferred_renderer->create_deferred_pass(win_width, win_height, component_interface, this);
			break;
		}
		default:
			LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
			break;
		}

		// initlaise all shaders and pipelines that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) {
			this->add_shader((RenderTypes)r_type, component_interface);
		}

		// setup environment rendering if needded
		init_environment_render();
	}
	
	void RenderInterface::init_environment_render()
	{

	}

	void RenderInterface::add_shader(RenderTypes type, std::unique_ptr<ComponentInterface>& component_interface)
	{
		VulkanAPI::Shader shader;
		switch (type) {
		case OmegaEngine::RenderTypes::Mesh:
			render_pipelines[(int)RenderTypes::Mesh] = RenderableMesh::create_mesh_pipeline(vk_interface->get_device(), renderer, component_interface);
			break;
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}

		render_pipelines[(int)type].shader = shader;
	}

	void RenderInterface::render_components(RenderConfig& render_config, VulkanAPI::RenderPass& renderpass, vk::Semaphore& image_semaphore, vk::Semaphore& component_semaphore)
	{
		RenderQueueInfo queue_info;
	
		for (auto& info : renderables) {
			
			switch (info.renderable->get_type()) {
			case RenderTypes::Mesh: {
				queue_info.renderable_handle = info.renderable->get_handle();
				queue_info.render_function = get_member_render_function<void, RenderableMesh, &RenderableMesh::render>;
				break;
			}
			}

			queue_info.renderable_data = info.renderable->get_instance_data();
			queue_info.sorting_key = info.renderable->get_sort_key();
			queue_info.queue_type = info.renderable->get_queue_type();

			render_queue->add_to_queue(queue_info);		
		}

		// sort by the set order - layer, shader, material and depth
		render_queue->sort_all();

		// now draw all renderables to the pass - start by begining the renderpass 
		cmd_buffer.create_primary(VulkanAPI::CommandBuffer::UsageType::Multi);
		vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
		cmd_buffer.begin_renderpass(begin_info, true);

		// now draw everything in the queue - TODO: add all renderpasses to the queue (offscreen stuff, etc.)
		render_queue->threaded_dispatch(cmd_buffer, this);

		// end the primary pass and buffer
		cmd_buffer.end_pass();
		cmd_buffer.end();

		// submit to graphics queue
		vk_interface->get_graph_queue().submit_cmd_buffer(cmd_buffer.get(), image_semaphore, component_semaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput);

		isDirty = true;
	}

	void RenderInterface::build_renderable_tree(Object& obj, std::unique_ptr<ComponentInterface>& comp_interface)
	{
		auto& mesh_manager = comp_interface->getManager<MeshManager>();

		uint32_t mesh_index = obj.get_manager_index<MeshManager>();
		MeshManager::Mesh* mesh = mesh_manager.get_mesh(mesh_index);

		// we need to add all the primitve sub meshes as renderables
		for (auto& primitive : mesh.primitives) {
			add_renderable<RenderableMesh>(comp_interface, mesh, primitive);
		}

		// and do the same for all children associated with this mesh
		auto children = obj.get_children();
		for (auto child : children) {
			build_renderable_tree(child, comp_interface);
		}
	}

	void RenderInterface::update_renderables(std::unique_ptr<ObjectManager>& object_manager, std::unique_ptr<ComponentInterface>& comp_interface)
	{
		if (isDirty) {

			// get all objects
			auto objects = object_manager->get_objects_list();

			for (auto& object : objects) {
				build_renderable_tree(object.second, comp_interface);
			}

			isDirty = false;
		}
	}

	void RenderInterface::render(double interpolation)
	{
		renderer->render(this, vk_interface);
	}

	void RenderInterface::prepare_swapchain_pass()
	{
		auto& swap_chain = vk_interface->get_swapchain();
		vk::Format sc_format = swap_chain.get_format();

		uint32_t width = swap_chain.get_extents_width();
		uint32_t height = swap_chain.get_extents_height();

		// depth image
		vk::Format depth_format = VulkanAPI::Device::get_depth_format(vk_interface->get_gpu());
		swapchain_present.depth_texture.create_empty_image(vk_interface->get_device(), vk_interface->get_gpu(), depth_format, width, height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		swapchain_present.renderpass.init(vk_interface->get_device());
		swapchain_present.renderpass.addAttachment(vk::ImageLayout::ePresentSrcKHR, sc_format);
		swapchain_present.renderpass.addAttachment(vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal, depth_format);

		swapchain_present.renderpass.addReference(vk::ImageLayout::eColorAttachmentOptimal, 0);
		swapchain_present.renderpass.addReference(vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
		swapchain_present.renderpass.prepareRenderPass();

		// create presentation renderpass/framebuffer for each swap chain image
		for (uint32_t i = 0; i < swap_chain.get_image_count(); ++i) {

			// create a frame buffer for each swapchain image
			std::vector<vk::ImageView> image_views{ swap_chain.get_image_view(i), swapchain_present.depth_texture.get_image_view() };
			swapchain_present.renderpass.prepareFramebuffer(static_cast<uint32_t>(image_views.size()), image_views.data(), width, height);
		}

		// a command buffer is required for each presentation image
		swapchain_present.cmd_buffer.resize(swap_chain.get_image_count());

		// might as well sort out the background clear colour now too
		swapchain_present.clear_values[0].color = static_cast<vk::ClearColorValue>(render_config.general.background_col);
		swapchain_present.clear_values[1].depthStencil = { 1.0f, 0 };
	}

	void RenderInterface::begin_swapchain_pass(uint32_t index)
	{
		auto& begin_info = swapchain_present.renderpass.get_begin_info(swapchain_present.clear_values.size(), swapchain_present.clear_values.data());
		swapchain_present.cmd_buffer[index].begin_renderpass(begin_info, index);

		swapchain_present.cmd_buffer[index].set_viewport();
		swapchain_present.cmd_buffer[index].set_scissor();
	}

	void RenderInterface::end_swapchain_pass(uint32_t index)
	{
		swapchain_present.cmd_buffer[index].end_pass();
		swapchain_present.cmd_buffer[index].end();
	}

}
