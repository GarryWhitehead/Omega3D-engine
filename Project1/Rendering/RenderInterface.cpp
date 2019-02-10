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

		// setup the renderer pipeline
		switch (static_cast<RendererType>(render_config.general.renderer)) {
		case RendererType::Deferred:
		{
			def_renderer = std::make_unique<DeferredRenderer>(device.getDevice(), device.getPhysicalDevice(), render_config);
			def_renderer->create(win_width, win_height, component_interface->getManager<CameraManager>());
			render_callback = def_renderer->set_render_callback(this, vk_interface);
			break;
		}
		default:
			LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
			break;
		}

		// initlaise all shaders and pipelines that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) {
			this->add_shader((RenderTypes)r_type);
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

	void RenderInterface::add_shader(RenderTypes type)
	{
		VulkanAPI::Shader shader;
		switch (type) {
		case OmegaEngine::RenderTypes::Mesh:
			render_pipelines[(int)RenderTypes::Mesh] = RenderableMesh::create_mesh_pipeline(vk_interface->get_device(), def_renderer);
			break;
		case OmegaEngine::RenderTypes::Skybox:
			shader.add(vk_interface->get_device(), "env/skybox.vert", VulkanAPI::StageType::Vertex, "env/skybox.frag", VulkanAPI::StageType::Fragment);
			break;
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}

		render_pipelines[(int)type].shader = shader;
	}

	void RenderInterface::render_components()
	{
		uint32_t num_threads = std::thread::hardware_concurrency();
		ThreadPool thread_pool(num_threads);

		uint32_t threads_per_group = VULKAN_THREADED_GROUP_SIZE / num_threads;

		for (auto& info : renderables) {

			// Note: this is in a specific order in whcih renderable should be drawn
			switch (info.renderable->get_type()) {
			case RenderTypes::Skybox:
				break;
			case RenderTypes::Mesh:
				dynamic_cast<RenderableMesh*>(info.renderable)->render(cmd_buffer, render_pipelines[(int)RenderTypes::Mesh], thread_pool, threads_per_group, num_threads);
				break;
			default:
				LOGGER_INFO("Error whilst rendering. Unsupported renderable detected.");
			}
		}

		// check that all threads are finshed before executing the cmd buffers
		thread_pool.wait_for_all();

		cmd_buffer.secondary_execute_commands();
		cmd_buffer.end();
	}

	void RenderInterface::add_mesh_tree(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj)
	{
		add_renderable<RenderableMesh>(vk_interface->get_device(), comp_interface, obj);

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
