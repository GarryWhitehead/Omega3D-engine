#include "World.h"
#include "Utility/logger.h"
#include "Utility/FileUtil.h"
#include "Engine/Omega_SceneParser.h"
#include "DataTypes/Camera.h"
#include "ComponentInterface/ComponentInterface.h"
#include "ComponentInterface/ObjectManager.h"
#include "Managers/LightManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "Managers/AnimationManager.h"
#include "Managers/TransformManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Threading/ThreadPool.h"
#include "Omega_Common.h"

namespace OmegaEngine
{
	
	World::World(Managers managers, VulkanAPI::Device device)
	{
		component_interface = std::make_unique<ComponentInterface>(device);
		render_interface = std::make_unique<RenderInterface>();

		// register all components managers required for this world
		if (managers & Managers::OE_MANAGERS_MESH || managers & Managers::OE_MANAGERS_ALL) {
			component_interface->registerManager<MeshManager>();

			// the mesh manager also requires the material and texture managers
			component_interface->registerManager<MaterialManager>();
			component_interface->registerManager<TextureManager>();
		}
		if (managers & Managers::OE_MANAGERS_ANIMATION || managers & Managers::OE_MANAGERS_ALL) {
			component_interface->registerManager<AnimationManager>();
		}
		if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL) {
			component_interface->registerManager<LightManager>();
		}
		if (managers & Managers::OE_MANAGERS_TRANSFORM || managers & Managers::OE_MANAGERS_ALL) {
			component_interface->registerManager<TransformManager>();
		}
	}

	World::~World()
	{
		
	}

	bool World::create(const char* filename)
	{
		SceneParser parser;
		if (!parser.parse(filename)) {
			throw std::runtime_error("Unable to parse omega engine scene file.");
		}

		// load and distribute the gltf data between the appropiate systems.
#ifdef OMEGA_ENGINE_THREADED
		ThreadPool threads(SCENE_LOAD_THREAD_COUNT);

		for (uint32_t i = 0; i < parser.modelCount(); ++i) {
			threads.submitTask([&parser, this, i]() { 
				this->addGltfData(parser.getFilenames(i), parser.getWorldMatrix(i)); 
			});
		}
#else
		for (uint32_t i = 0; i < parser.modelCount(); ++i) {
				component_interface->addGltfData(parser.getFilenames(i), parser.getWorldMatrix(i));
		}
#endif
	}

	void World::update(double time, double dt)
	{
		component_interface->update_managers(time, dt);
	}

	void World::render(double interpolation)
	{
		render_interface->render(interpolation);
	}

	void World::addGltfData(const char* filename, OEMaths::mat4f world_mat)
	{
		// open the gltf file
		tinygltf::Model model;
		tinygltf::TinyGLTF loader;

		std::string err, warn;
		std::string ext;

		FileUtil::GetFileExtension(filename, ext);
		bool ret = false;
		if (ext.compare("glb") == 0) {
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
		}
		else {
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		}

		if (ret) {

			// first get all materials and textures associated with this model
			for (auto& tex : model.textures) {
				tinygltf::Image image = model.images[tex.source];
				component_interface->getManager<TextureManager>()->addGltfImage(image);
			}

			for (auto& mat : model.materials) {
				component_interface->getManager<MaterialManager>()->addGltfMaterial(mat);
			}

			// we are going to parse the node recursively to get all the info required for the space - this will add a new object per node - which are treated as models.
			// data will be passed to all the relevant managers for this object and components added automatically
			tinygltf::Scene &scene = model.scenes[model.defaultScene];
			for (uint32_t i = 0; i < scene.nodes.size(); ++i) {

				Object obj = objectManager->createObject();

				tinygltf::Node node = model.nodes[scene.nodes[i]];
		
				loadGltfNode(model, node, world_mat, objectManager, obj, false);

				// add as renderable targets
				render_interface->addObjectAndChildren<RenderableMesh>(component_interface, obj);
			}
		}
		else {
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err);
			throw std::runtime_error("Unable to parse gltf file.");
		}
	}

	void World::loadGltfNode(tinygltf::Model& model, tinygltf::Node& node, OEMaths::mat4f world_transform, std::unique_ptr<ObjectManager>& objManager, Object& obj, bool childObject)
	{
		NodeInfo newNode;

		newNode.parentIndex = parentNode;
		newNode.name = node.name.c_str();
		newNode.skinIndex = node.skin;

		// add all local and world transforms to the transform manager 
		auto &transform_man = component_interface->getManager<TransformManager>();
		transform_man->addGltfTransform(node, obj, world_transform);

		Object parentObject;
		if (childObject) {
			parentObject = objManager->createChildObject(obj);
		}
		else {
			parentObject = obj;
		}

		// if this node has children, recursively extract their info
		if (!node.children.empty()) {
			for (uint32_t i = 0; node.children.size(); ++i) {
				loadGltfNode(model, model.nodes[node.children[i]], world_transform, objManager, parentObject, true);
			}
		}

		// if the node has mesh data...
		if (node.mesh > -1) {
			auto& mesh_manager = component_interface->getManager<MeshManager>();
			mesh_manager->addGltfData(model, node, obj);
			
		}
	}



	

}