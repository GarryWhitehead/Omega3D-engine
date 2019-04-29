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
#include "Managers/MaterialManager.h"
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

	RenderInterface::RenderInterface(VulkanAPI::Device& device, std::unique_ptr<ComponentInterface>& component_interface, const uint32_t width, const uint32_t height)
	{
		init(device, width, height);
	}


	RenderInterface::~RenderInterface()
	{
	}

	void RenderInterface::init(VulkanAPI::Device& device, const uint32_t width, const uint32_t height)
	{
		// load the render config file if it exsists
		load_render_config();

		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, width, height);
		
		// all renderable elements will be dispatched for drawing via this queue
		render_queue = std::make_unique<RenderQueue>();
		
		// init the command buffer now ready for rendering later
		cmd_buffer_handle = vk_interface->get_cmd_buffer_manager()->create_instance();
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
		// setup the renderer pipeline
		switch (static_cast<RendererType>(render_config.general.renderer)) {
		case RendererType::Deferred:
		{
			set_renderer<DeferredRenderer>(vk_interface->get_device(), vk_interface->get_gpu(), vk_interface->get_cmd_buffer_manager(), render_config);
			auto deferred_renderer = reinterpret_cast<DeferredRenderer*>(renderer.get());
			deferred_renderer->create_gbuffer_pass();
			deferred_renderer->create_deferred_pass(vk_interface->get_buffer_manager(), vk_interface->get_swapchain());
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
		auto& state = std::make_unique<ProgramState>();

		switch (type) {
		case OmegaEngine::RenderTypes::StaticMesh: {
			 RenderableMesh::create_mesh_pipeline(vk_interface->get_device(),
				renderer, vk_interface->get_buffer_manager(), vk_interface->get_texture_manager(), MeshManager::MeshType::Static, state);
			 render_states[(int)RenderTypes::StaticMesh] = std::move(state);
			break;
		}
		case OmegaEngine::RenderTypes::SkinnedMesh: {
			RenderableMesh::create_mesh_pipeline(vk_interface->get_device(),
				renderer, vk_interface->get_buffer_manager(), vk_interface->get_texture_manager(), MeshManager::MeshType::Skinned, state);
			render_states[(int)RenderTypes::SkinnedMesh] = std::move(state);
			break;
		}
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}
	}

	void RenderInterface::render_components(RenderConfig& render_config, VulkanAPI::RenderPass& renderpass)
	{
		RenderQueueInfo queue_info;

		auto& cmd_buffer_manager = vk_interface->get_cmd_buffer_manager();

		// if it's a static scene and we've already recorded the buffer then don't do anything else
		if (scene_mode == SceneMode::Static && cmd_buffer_manager->is_recorded(cmd_buffer_handle)) {
			return;
		}

		cmd_buffer_manager->new_frame(cmd_buffer_handle);
		auto& cmd_buffer = cmd_buffer_manager->get_cmd_buffer(cmd_buffer_handle);

		for (auto& info : renderables) {

			switch (info.renderable->get_type()) {
			case RenderTypes::SkinnedMesh:
			case RenderTypes::StaticMesh: {
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
		cmd_buffer->create_primary();
		vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
		cmd_buffer->begin_renderpass(begin_info, true);

		// now draw everything in the queue - TODO: add all renderpasses to the queue (offscreen stuff, etc.)
		render_queue->threaded_dispatch(cmd_buffer, this);

		// end the primary pass and buffer
		cmd_buffer->end_pass();
		cmd_buffer->end();
	}

	void RenderInterface::build_renderable_tree(Object& obj, std::unique_ptr<ComponentInterface>& comp_interface)
	{
		auto& mesh_manager = comp_interface->getManager<MeshManager>();

		// make sure that this object has a mesh
		if (obj.hasComponent<MeshManager>()) {

			uint32_t mesh_index = obj.get_manager_index<MeshManager>();
			MeshManager::StaticMesh mesh = mesh_manager.get_mesh(mesh_index);

			// we need to add all the primitve sub meshes as renderables
			for (auto& primitive : mesh.primitives) {
				add_renderable<RenderableMesh>(vk_interface->get_device(), comp_interface, vk_interface->get_buffer_manager(), 
						vk_interface->get_texture_manager(), mesh, primitive, obj);
			}
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
		// update buffers before doing the rendering
		vk_interface->get_buffer_manager()->update();

		renderer->render(this, vk_interface);
	}

}
