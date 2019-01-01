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
#include "Rendering/RenderManager.h"
#include "Threading/ThreadPool.h"
#include "Omega_Common.h"

namespace OmegaEngine
{
	
	World::World(Managers managers)
	{
		compSystem = std::make_unique<ComponentInterface>();
		renderManager = std::make_unique<RenderManager>();

		// register all components managers required for this world
		if (managers & Managers::OE_MANAGERS_MESH || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<MeshManager>();
		}
		if (managers & Managers::OE_MANAGERS_ANIMATION || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<AnimationManager>();
		}
		if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<LightManager>();
		}
		if (managers & Managers::OE_MANAGERS_TRANSFORM || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<TransformManager>();
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
				compSystem->addGltfData(parser.getFilenames(i), parser.getWorldMatrix(i));
		}
#endif
	}

	void World::update()
	{
		// Check whether new spaces need to be loaded into memory or removed - if so, do this on spertate threads
		// this depends on the max number of spaces that can be hosted on the CPU - determined by mem size
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

			// we are going to parse the node recursively to get all the info required for the space - this will add a new object per node - which are treated as models.
			// data will be passed to all the relevant managers for this object and components added automatically
			tinygltf::Scene &scene = model.scenes[model.defaultScene];
			for (uint32_t i = 0; i < scene.nodes.size(); ++i) {

				Object obj = objectManager->createObject();

				tinygltf::Node node = model.nodes[scene.nodes[i]];
		
				loadGltfNode(model, node, world_mat, objectManager, obj, false);

				// add as renderable targets
				renderManager->getInterface()->addRenderable(compSystem, obj);
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
		auto &transform_man = compSystem->getManager<TransformManager>();
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
			auto& mesh_manager = compSystem->getManager<MeshManager>();
			mesh_manager->addGltfData(model, node, obj);
			
		}
	}



	

}