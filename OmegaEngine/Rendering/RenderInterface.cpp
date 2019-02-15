#include "RenderInterface.h"
#include "RenderableTypes/RenderableBase.h"
#include "RenderableTypes/Mesh.h"
#include "Rendering/DeferredRenderer.h"
#include "Managers/ComponentInterface.h"
#include "PostProcess/PostProcessInterface.h"
#include "Objects/Object.h"
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

	RenderInterface::RenderInterface(VulkanAPI::Device device, std::unique_ptr<ComponentInterface>& component_interface)
	{
		// load the render config file if it exsists
		load_render_config();

		uint32_t win_width = Global::program_state.get_win_width();
		uint32_t win_height = Global::program_state.get_win_height();

		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, win_width, win_height);
		
		render_queue = std::make_unique<RenderQueue>();
		
		// setup environment rendering if needded
		init_environment_render();

		// create renderer - only deferred renderer supported at the mo
		init_renderer(component_interface);

		// initlaise all shaders and pipelines that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) {
			this->add_shader((RenderTypes)r_type, component_interface);
		}
	}


	RenderInterface::~RenderInterface()
	{
	}

	void RenderInterface::load_render_config()
	{
		std::string json;
		const char filename[] = "render_config.ini";		// probably need to check the current dir here
		if (!FileUtil::readFileIntoBuffer(filename, json)) {
			return;
		}

		// if we cant parse the confid, then go with the default values
		rapidjson::Document doc;
		if (doc.Parse(json.c_str()).HasParseError()) {
			LOGGER_INFO("Unable to find render_config file. Using default settings...")
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
			def_renderer = std::make_unique<DeferredRenderer>(vk_interface->get_device(), vk_interface->get_gpu(), render_config);
			def_renderer->create(win_width, win_height, component_interface->getManager<CameraManager>());
			render_callback = def_renderer->set_render_callback(this, vk_interface);
			break;
		}
		default:
			LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
			break;
		}
	}
	
	void RenderInterface::init_environment_render()
	{

	}

	void RenderInterface::add_shader(RenderTypes type, std::unique_ptr<ComponentInterface>& component_interface)
	{
		VulkanAPI::Shader shader;
		switch (type) {
		case OmegaEngine::RenderTypes::Mesh:
			render_pipelines[(int)RenderTypes::Mesh] = RenderableMesh::create_mesh_pipeline(vk_interface->get_device(), def_renderer, component_interface);
			break;
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}

		render_pipelines[(int)type].shader = shader;
	}

	void RenderInterface::render_components()
	{
		// set the command buffer that will be used for the queue
		render_queue->add_cmd_buffer(cmd_buffer);
		
		RenderQueueInfo queue_info;
	
		for (auto& info : renderables) {
			
			queue_info.render_function = &info.renderable->render;
			queue_info.renderable_data = info.renderable->get_instance_data();
			queue_info.sorting_key = info.renderable->get_sort_key();
			queue_info.queue_type = info.renderable->get_queue_type();

			render_queue->add_to_queue(queue_info);		
		}
	}

	void RenderInterface::add_mesh_tree(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj)
	{
		auto& mesh_manager->getManager<MeshManager>();

		uint32_t mesh_index = obj.get_manager_index<MeshManager>();
		MeshManager::StaticMesh mesh = mesh_man.get_mesh(mesh_index);

		// we need to add all the primitve sub meshes as renderables
		for (auto& primitive: mesh.primitives) {
			add_renderable<RenderableMesh>(vk_interface->get_device(), comp_interface, mesh, primitive);
		}

		// and do the same for all children associated with this mesh
		auto& children = obj.get_children();
		for (auto& child : children) {
			add_mesh_tree(comp_interface, child);
		}
	}

	void RenderInterface::render(double interpolation)
	{
		render_callback();
	}

}
