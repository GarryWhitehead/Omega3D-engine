#include "World.h"
#include "Utility/logger.h"
#include "Utility/FileUtil.h"
#include "Engine/Omega_SceneParser.h"
#include "Engine/Omega_Config.h"
#include "Engine/Omega_Global.h"
#include "Objects/Object.h"
#include "Managers/ComponentInterface.h"
#include "Objects/ObjectManager.h"
#include "Managers/LightManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/AnimationManager.h"
#include "Managers/TransformManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Managers/CameraManager.h"
#include "Managers/EventManager.h"
#include "Managers/AssetManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Threading/ThreadPool.h"
#include "Utility/BVH.hpp"
#include "Omega_Common.h"
#include "Vulkan/Device.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

// tiny gltf uses window.h - this stops the macros messing with std::min
#define NOMINMAX

#include "tiny_gltf.h"

namespace OmegaEngine
{
	World::World()
	{

	}

	World::World(Managers managers, std::unique_ptr<VulkanAPI::Device>& device, EngineConfig& engine_config)
	{
		// all the boiler plater needed to generate the manager and interface instances
		objectManager = std::make_unique<ObjectManager>();
		component_interface = std::make_unique<ComponentInterface>();
		render_interface = std::make_unique<RenderInterface>(device, engine_config.screen_width, engine_config.screen_height, static_cast<SceneType>(engine_config.scene_type));
		animation_manager = std::make_unique<AnimationManager>();
		asset_manager = std::make_unique<AssetManager>();
		bvh = std::make_unique<BVH>();

		// register all components managers required for this world
		if (managers & Managers::OE_MANAGERS_MESH || managers & Managers::OE_MANAGERS_ALL)
		{
			component_interface->registerManager<MeshManager>();
		}
		if (managers & Managers::OE_MANAGERS_MATERIAL || managers & Managers::OE_MANAGERS_ALL) 
		{
			component_interface->registerManager<MaterialManager>();
		}
		if (managers & Managers::OE_MANAGERS_TEXTURE || managers & Managers::OE_MANAGERS_ALL) 
		{
			component_interface->registerManager<TextureManager>();
		}
		if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL) 
		{
			component_interface->registerManager<LightManager>();
		}
		if (managers & Managers::OE_MANAGERS_TRANSFORM || managers & Managers::OE_MANAGERS_ALL)
		{
			component_interface->registerManager<TransformManager>();
		}
		if (managers & Managers::OE_MANAGERS_CAMERA || managers & Managers::OE_MANAGERS_ALL) 
		{
			component_interface->registerManager<CameraManager>(engine_config.mouse_sensitivity);
		}
		
		// setup the preferred renderer and associated elements
		render_interface->init_renderer(component_interface);
	}

	World::~World()
	{
		
	}

	bool World::create(const char* filename)
	{
		SceneParser parser;
		if (!parser.parse(filename))
		{
			return false;
		}

		// update camera manager 
		component_interface->getManager<CameraManager>().add_camera(parser.get_camera());

		// add lights from scene file
		for (uint32_t i = 0; i < parser.light_count(); ++i) 
		{
			component_interface->getManager<LightManager>().add_light(parser.get_light(i));
		}

		// environment - load skybox and IBL files into memory if they exsist
		if (parser.get_environment().skybox_filename)
		{
			asset_manager->load_image_file(parser.get_environment().skybox_filename);

			// add skybox as a object - TODO: blur factor should be obtained from the config settings
			auto obj = objectManager->createObject();
			obj->add_component<SkyboxComponent>(0.5f);
		}
		if (parser.get_environment().brdf_filename)
		{
			asset_manager->load_image_file(parser.get_environment().brdf_filename);
		}
		if (parser.get_environment().irradiance_map_filename)
		{
			asset_manager->load_image_file(parser.get_environment().irradiance_map_filename);
		}

		// load and distribute the gltf data between the appropiate systems.
		for (uint32_t i = 0; i < parser.modelCount(); ++i) 
		{
			this->addGltfData(parser.getFilenames(i), parser.getWorldMatrix(i));
		}

		return true;
	}

	void World::update(double time, double dt)
	{
		// update on a per-frame basis
		// animation
		animation_manager->update_anim(time, dt, component_interface->getManager<TransformManager>());

		// newly added assets need to be hosted on the gpu
		asset_manager->update();

		// all other managers
		component_interface->update_managers(time, dt, objectManager);

		// check whether there are any queued events to deal with
		Global::eventManager()->notifyQueued();

		// add all objects as renderable targets unless they flagged otherwise 
		render_interface->update_renderables(objectManager, component_interface);

		has_updated_once = true;
	}

	void World::render(double interpolation)
	{
		if (has_updated_once) 
		{
			render_interface->render(interpolation);
		}
	}

	void World::addGltfData(std::string filename, OEMaths::mat4f world_mat)
	{
		// open the gltf file
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;

		std::string err, warn;
		std::string ext;

		FileUtil::GetFileExtension(filename, ext);
		bool ret = false;
		if (ext.compare("glb") == 0) 
		{
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str());
		}
		else 
		{
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename.c_str());
		}

		if (ret) 
		{
			auto& texture_manager = component_interface->getManager<TextureManager>();
			auto& material_manager = component_interface->getManager<MaterialManager>();

			uint32_t set = texture_manager.get_current_set();

			// first get all materials and textures associated with this model
			for (auto& tex : model.textures)
			{
				tinygltf::Image image = model.images[tex.source];
				texture_manager.addGltfImage(image);
			}

			for (auto& sampler : model.samplers) 
			{
				texture_manager.addGltfSampler(set, sampler);
			}

			for (auto& mat : model.materials) 
			{
				material_manager.addGltfMaterial(set, mat, texture_manager);
			}
			texture_manager.next_set();

			// we are going to parse the node recursively to get all the info required for the space - this will add a new object per node - which are treated as models.
			// data will be passed to all the relevant managers for this object and components added automatically
			tinygltf::Scene &scene = model.scenes[model.defaultScene];

			uint32_t vertex_count = 0;
			uint32_t index_count = 0;

			// we also need to keep a linerised form of the objects for matching up joint indices later
			std::unordered_map<uint32_t, Object> linearised_objects;
			for (uint32_t i = 0; i < scene.nodes.size(); ++i) 
			{
				Object* obj = objectManager->createObject();

				tinygltf::Node node = model.nodes[scene.nodes[i]];
				loadGltfNode(model, node, linearised_objects, world_mat, objectManager, obj, false, scene.nodes[i], vertex_count, index_count);
			}

			// skinning info
			component_interface->getManager<TransformManager>().addGltfSkin(model, linearised_objects);

			// animation
			animation_manager->addGltfAnimation(model, linearised_objects);
		}
		else 
		{
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err.c_str());
		}
	}

	void World::loadGltfNode(tinygltf::Model& model, tinygltf::Node& node, 
							std::unordered_map<uint32_t, Object>& linearised_objects, 
							OEMaths::mat4f world_transform, 
							std::unique_ptr<ObjectManager>& objManager, 
							Object* obj, bool childObject,
							uint32_t node_index,
							uint32_t& global_vertex_count, uint32_t& global_index_count)
	{
		// TODO: rather than store the objects in the actual parent, store these as part of the object list and only store the
		// indice to these objects in the parent. This will then allow these meshes to be used by other objects.
		Object* parentObject;
		if (childObject) 
		{
			parentObject = objManager->createChildObject(*obj);
		}
		else 
		{
			parentObject = obj;
		}

		// add all local and world transforms to the transform manager - also combines skinning info
		auto &transform_man = component_interface->getManager<TransformManager>();
		transform_man.addGltfTransform(node, parentObject, world_transform);

		// if this node has children, recursively extract their info
		if (node.children.size() > 0) 
		{
			for (uint32_t i = 0; i < node.children.size(); ++i) 
			{
				loadGltfNode(model, model.nodes[node.children[i]], linearised_objects, world_transform, objManager, 
					parentObject, true, node.children[i], global_vertex_count, global_index_count);
			}
		}

		// if the node has mesh data...
		if (node.mesh > -1) 
		{
			auto& mesh_manager = component_interface->getManager<MeshManager>();
			mesh_manager.addGltfData(model, node, parentObject, global_vertex_count, global_index_count);
		}

		// create the linearised list of objects - parents and children
		linearised_objects[node_index] = *parentObject;
	}

}