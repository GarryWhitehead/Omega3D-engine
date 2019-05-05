#include "RenderInterface.h"
#include "RenderableTypes/RenderableBase.h"
#include "RenderableTypes/Mesh.h"
#include "RenderableTypes/Skybox.h"
#include "RenderableTypes/Shadow.h"
#include "Rendering/RenderQueue.h"
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
#include "Engine/World.h"
#include "Vulkan/VkTextureManager.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"


namespace OmegaEngine
{
	RenderInterface::RenderInterface()
	{
	}

	RenderInterface::RenderInterface(VulkanAPI::Device& device, std::unique_ptr<ComponentInterface>& component_interface, const uint32_t width, const uint32_t height, SceneType type) :
		scene_type(type)
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

		// all renderable elements will be dispatched for drawing via this queue
		render_queue = std::make_unique<RenderQueue>();

		// initiliase the graphical backend - we are solely using Vulkan
		// The new frame mode depends on tehe scene type - static scenes will only have their cmd buffers recorded
		// to once whilst dynamic scenes will be recorded to on a per frame basis.
		VulkanAPI::NewFrameMode mode;
		if (scene_type == SceneType::Static)
		{
			mode = VulkanAPI::NewFrameMode::Static;
		}
		else 
		{
			// this could be either the renewal or resetting of cmd buffers - what;'s best needs to be checked
			mode = VulkanAPI::NewFrameMode::Reset;
		}

		// create the vulkan API interface - this is the middle man between the renderer and the vulkan backend
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, width, height, mode);
	}

	void RenderInterface::load_render_config()
	{
		std::string json;
		const char filename[] = "render_config.ini";		// probably need to check the current dir here
		if (!FileUtil::readFileIntoBuffer(filename, json)) 
		{
			return;
		}

		// if we cant parse the config, then go with the default values
		rapidjson::Document doc;
		if (doc.Parse(json.c_str()).HasParseError())
		{
			LOGGER_INFO("Unable to find render_config file. Using default settings...");
			return;
		}

		// general render settings
		if (doc.HasMember("Renderer")) 
		{
			int renderer = doc["Renderer"].GetInt();
			render_config.general.renderer = static_cast<RendererType>(renderer);
		}
		
	}

	void RenderInterface::init_renderer(std::unique_ptr<ComponentInterface>& component_interface)
	{
		// setup the renderer pipeline
		switch (static_cast<RendererType>(render_config.general.renderer)) 
		{
		case RendererType::Deferred:
		{
			set_renderer<DeferredRenderer>(vk_interface->get_device(), vk_interface->get_gpu(), vk_interface->get_cmd_buffer_manager(), 
				render_config, vk_interface->get_buffer_manager(), vk_interface->get_swapchain());
			break;
		}
		default:
			LOGGER_ERROR("Using a unsupported rendering pipeline. At the moment only deferred shader is supported.");
			break;
		}

		// initlaise all shaders and pipelines that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) 
		{
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

		switch (type) 
		{
		case OmegaEngine::RenderTypes::StaticMesh:
		{
			 RenderableMesh::create_mesh_pipeline(vk_interface->get_device(),
				renderer, vk_interface->get_buffer_manager(), vk_interface->get_texture_manager(), MeshManager::MeshType::Static, state);
			 render_states[(int)RenderTypes::StaticMesh] = std::move(state);
			break;
		}
		case OmegaEngine::RenderTypes::SkinnedMesh: 
		{
			RenderableMesh::create_mesh_pipeline(vk_interface->get_device(),
				renderer, vk_interface->get_buffer_manager(), vk_interface->get_texture_manager(), MeshManager::MeshType::Skinned, state);
			render_states[(int)RenderTypes::SkinnedMesh] = std::move(state);
			break;
		}
		case OmegaEngine::RenderTypes::ShadowStatic:
		{
			RenderableShadow::create_shadow_pipeline(vk_interface->get_device(), renderer, vk_interface->get_buffer_manager(), state, MeshManager::MeshType::Static);
			render_states[(int)RenderTypes::ShadowStatic] = std::move(state);
			break;
		}
		case OmegaEngine::RenderTypes::ShadowDynamic:
		{
			RenderableShadow::create_shadow_pipeline(vk_interface->get_device(), renderer, vk_interface->get_buffer_manager(), state, MeshManager::MeshType::Skinned);
			render_states[(int)RenderTypes::ShadowDynamic] = std::move(state);
			break;
		}
		case OmegaEngine::RenderTypes::Skybox: 
		{
			RenderableSkybox::create_skybox_pipeline(vk_interface->get_device(),
				renderer, vk_interface->get_buffer_manager(), vk_interface->get_texture_manager(), state);
			render_states[(int)RenderTypes::Skybox] = std::move(state);
			break;
		}
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}
	}

	void RenderInterface::build_renderable_mesh_tree(Object& obj, std::unique_ptr<ComponentInterface>& comp_interface, bool is_shadow)
	{
		auto& mesh_manager = comp_interface->getManager<MeshManager>();

		MeshComponent comp = obj.get_component<MeshComponent>();
		MeshManager::StaticMesh mesh = mesh_manager.get_mesh(comp);

		// we need to add all the primitve sub meshes as renderables
		for (auto& primitive : mesh.primitives) 
		{
			if (!is_shadow)
			{
				add_renderable<RenderableMesh>(vk_interface->get_device(), comp_interface, vk_interface->get_buffer_manager(),
					vk_interface->get_texture_manager(), mesh, primitive, obj);
			}
			else
			{
				add_renderable<RenderableShadow>(vk_interface->get_device(), comp_interface, vk_interface->get_buffer_manager(),
					vk_interface->get_texture_manager(), mesh, primitive, obj);
			}
		}
	
		// and do the same for all children associated with this mesh
		auto children = obj.get_children();
		for (auto child : children) 
		{
			build_renderable_mesh_tree(child, comp_interface);
		}
	}

	void RenderInterface::update_renderables(std::unique_ptr<ObjectManager>& object_manager, std::unique_ptr<ComponentInterface>& comp_interface)
	{
		if (isDirty)
		{
			// get all objects
			auto& objects = object_manager->get_objects_list();

			for (auto& object : objects) 
			{
				if (object.second.hasComponent<MeshComponent>())
				{
					build_renderable_mesh_tree(object.second, comp_interface, false);
				}
				if (object.second.hasComponent<ShadowComponent>())
				{
					build_renderable_mesh_tree(object.second, comp_interface, true);
				}
				if (object.second.hasComponent<SkyboxComponent>())
				{
					add_renderable<RenderableSkybox>(this, object.second.get_component<SkyboxComponent>());
				}
			}

			isDirty = false;
		}
	}

	void RenderInterface::prepare_object_queue()
	{
		RenderQueueInfo queue_info;

		for (auto& info : renderables) {

			switch (info.renderable->get_type()) 
			{
				case RenderTypes::SkinnedMesh:
				case RenderTypes::StaticMesh: 
				{
					queue_info.renderable_handle = info.renderable->get_handle();
					queue_info.render_function = get_member_render_function<void, RenderableMesh, &RenderableMesh::render>;
					break;
				}
				case RenderTypes::ShadowStatic: 
				case RenderTypes::ShadowDynamic:
				{
					queue_info.renderable_handle = info.renderable->get_handle();
					queue_info.render_function = get_member_render_function<void, RenderableShadow, &RenderableShadow::render>;
					break;
				}
				case RenderTypes::Skybox: 
				{
					queue_info.renderable_handle = info.renderable->get_handle();
					queue_info.render_function = get_member_render_function<void, RenderableSkybox, &RenderableSkybox::render>;
					break;
				}
			}

			queue_info.renderable_data = info.renderable->get_instance_data();
			queue_info.sorting_key = info.renderable->get_sort_key();
			queue_info.queue_type = info.renderable->get_queue_type();

			render_queue->add_to_queue(queue_info);
		}
	}

	void RenderInterface::render(double interpolation)
	{
		// update buffer and texture descriptors before doing the rendering
		vk_interface->get_buffer_manager()->update();
		vk_interface->get_texture_manager()->update();

		prepare_object_queue();
		
		renderer->render(vk_interface, scene_type);
	}

}
