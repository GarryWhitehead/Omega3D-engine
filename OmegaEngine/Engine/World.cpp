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

	World::World(Managers managers, std::unique_ptr<VulkanAPI::Device>& device, EngineConfig& engineConfig)
	{
		// all the boiler plater needed to generate the manager and interface instances
		objectManager = std::make_unique<ObjectManager>();
		componentInterface = std::make_unique<ComponentInterface>();
		renderInterface = std::make_unique<RenderInterface>(device, engineConfig.screenWidth, engineConfig.screenHeight, static_cast<SceneType>(engineConfig.sceneType));
		animation_manager = std::make_unique<AnimationManager>();
		assetManager = std::make_unique<AssetManager>();
		bvh = std::make_unique<BVH>();

		// register all components managers required for this world
		if (managers & Managers::OE_MANAGERS_MESH || managers & Managers::OE_MANAGERS_ALL)
		{
			componentInterface->registerManager<MeshManager>();
		}
		if (managers & Managers::OE_MANAGERS_MATERIAL || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<MaterialManager>();
		}
		if (managers & Managers::OE_MANAGERS_TEXTURE || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<TextureManager>();
		}
		if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<LightManager>();
		}
		if (managers & Managers::OE_MANAGERS_TRANSFORM || managers & Managers::OE_MANAGERS_ALL)
		{
			componentInterface->registerManager<TransformManager>();
		}
		if (managers & Managers::OE_MANAGERS_CAMERA || managers & Managers::OE_MANAGERS_ALL) 
		{
			componentInterface->registerManager<CameraManager>(engineConfig.mouseSensitivity);
		}
		
		// setup the preferred renderer and associated elements
		renderInterface->initRenderer(componentInterface);
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
		componentInterface->getManager<CameraManager>().addCamera(parser.getCamera());

		// add lights from scene file
		for (uint32_t i = 0; i < parser.lightCount(); ++i) 
		{
			componentInterface->getManager<LightManager>().addLight(parser.getLights(i));
		}

		// environment - load skybox and IBL files into memory if they exsist
		if (parser.getEnvironment().skyboxFilename)
		{
			assetManager->loadImageFile(parser.getEnvironment().skyboxFilename);

			// add skybox as a object - TODO: blur factor should be obtained from the config settings
			auto obj = &objectManager->createObject();
			obj->addComponent<SkyboxComponent>(0.5f);
		}
		if (parser.getEnvironment().brdfFilename)
		{
			assetManager->loadImageFile(parser.getEnvironment().brdfFilename);
		}
		if (parser.getEnvironment().irradianceMapFilename)
		{
			assetManager->loadImageFile(parser.getEnvironment().irradianceMapFilename);
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
		animation_manager->updateAnimation(time, dt, componentInterface->getManager<TransformManager>());

		// newly added assets need to be hosted on the gpu
		assetManager->update();

		// all other managers
		componentInterface->update_managers(time, dt, objectManager);

		// check whether there are any queued events to deal with
		Global::eventManager()->notifyQueued();

		// add all objects as renderable targets unless they flagged otherwise 
		renderInterface->updateRenderables(objectManager, componentInterface);

		hasUpdatedOnce = true;
	}

	void World::render(double interpolation)
	{
		if (hasUpdatedOnce) 
		{
			renderInterface->render(interpolation);
		}
	}

	void World::addGltfData(std::string filename, OEMaths::mat4f worldMatix)
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
			auto& textureManager = componentInterface->getManager<TextureManager>();
			auto& materialManager = componentInterface->getManager<MaterialManager>();

			uint32_t set = textureManager.getCurrentSet();

			// first get all materials and textures associated with this model
			for (auto& tex : model.textures)
			{
				tinygltf::Image image = model.images[tex.source];
				textureManager.addGltfImage(image);
			}

			for (auto& sampler : model.samplers) 
			{
				textureManager.addGltfSampler(set, sampler);
			}

			for (auto& mat : model.materials) 
			{
				materialManager.addGltfMaterial(set, mat, textureManager);
			}
			textureManager.nextSet();

			// we are going to parse the node recursively to get all the info required for the space - this will add a new object per node - which are treated as models.
			// data will be passed to all the relevant managers for this object and components added automatically
			tinygltf::Scene &scene = model.scenes[model.defaultScene];;

			// we also need to keep a linerised form of the objects for matching up joint indices later
			std::unordered_map<uint32_t, Object> linearisedObjects;
			for (uint32_t i = 0; i < scene.nodes.size(); ++i) 
			{
				loadGltfNode(model, linearisedObjects, worldMatix, nullptr, scene.nodes[i]);
			}

			// skinning info
			componentInterface->getManager<TransformManager>().addGltfSkin(model, linearisedObjects);

			// animation
			animation_manager->addGltfAnimation(model, linearisedObjects);
		}
		else 
		{
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err.c_str());
		}
	}

	void World::loadGltfNode(tinygltf::Model& model,
							std::unordered_map<uint32_t, Object>& linearisedObjects, 
							OEMaths::mat4f worldTransform, 
							Object* parentObject,
							uint32_t nodeIndex)
	{
		tinygltf::Node node = model.nodes[nodeIndex];

		// TODO: rather than store the objects in the actual parent, store these as part of the object list and only store the
		// indices to these objects in the parent. This will then allow these meshes to be used by other objects.
		Object* childObject = nullptr;
		if (!parentObject)
		{
			childObject = &objectManager->createObject();
			
		}
		else 
		{
			childObject = &objectManager->createChildObject(*parentObject);
		}

		// add all local and world transforms to the transform manager - also combines skinning info
		auto &transformManager = componentInterface->getManager<TransformManager>();
		transformManager.addGltfTransform(node, childObject, worldTransform);

		// if this node has children, recursively extract their info
		if (node.children.size() > 0) 
		{
			for (uint32_t i = 0; i < node.children.size(); ++i) 
			{
				loadGltfNode(model,linearisedObjects, worldTransform, 
					childObject, node.children[i]);
			}
		}

		// if the node has mesh data...
		if (node.mesh > -1) 
		{
			auto& meshManager = componentInterface->getManager<MeshManager>();
			meshManager.addGltfMesh(model, node, childObject);
		}

		// create the linearised list of objects - parents and children
		linearisedObjects[nodeIndex] = *childObject;
	}

}